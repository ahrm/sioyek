#include "pdf_renderer.h"

PdfRenderer::PdfRenderer(fz_context* context_to_clone) : context_to_clone(context_to_clone) {
	invalidate_pointer = nullptr;
	mupdf_context = nullptr;
}

void PdfRenderer::init_context() {
	//should only be called from the worker thread
	if (mupdf_context == nullptr) {
		mupdf_context = fz_clone_context(context_to_clone);
		context_to_clone = nullptr;
	}
}

void PdfRenderer::set_invalidate_pointer(bool* inv_p) {
	invalidate_pointer = inv_p;
}

//should only be called from the main thread

void PdfRenderer::add_request(string document_path, int page, float zoom_level) {
	//fz_document* doc = get_document_with_path(document_path);
	if (document_path.size() > 0) {
		RenderRequest req;
		req.type = REQUEST_RENDER;
		req.path = document_path;
		req.page = page;
		req.zoom_level = zoom_level;

		pending_requests_mutex.lock();
		bool should_add = true;
		for (int i = 0; i < pending_requests.size(); i++) {
			if (pending_requests[i] == req) {
				should_add = false;
			}
		}
		if (should_add) {
			pending_requests.push_back(req);
		}
		if (pending_requests.size() > max_pending_requests) {
			pending_requests.erase(pending_requests.begin());
		}
		pending_requests_mutex.unlock();
	}
	else {
		cout << "Error: could not find documnet" << endl;
	}
}
void PdfRenderer::add_request(string document_path, int page, string term, vector<SearchResult>* out,float* percent_done, bool* is_searching, mutex* mut) {
	//fz_document* doc = get_document_with_path(document_path);
	if (document_path.size() > 0) {

		RenderRequest req;
		req.type = REQUEST_SEARCH;
		req.path = document_path;
		req.page = page;
		req.search_term = term;
		req.search_results_mutex = mut;
		req.search_results = out;
		req.percent_done = percent_done;
		req.is_searching = is_searching;

		pending_requests_mutex.lock();
		pending_requests.push_back(req);
		if (pending_requests.size() > max_pending_requests) {
			pending_requests.erase(pending_requests.begin());
		}
		pending_requests_mutex.unlock();
	}
	else {
		cout << "Error: could not find document" << endl;
	}
}

//should only be called from the main thread

GLuint PdfRenderer::find_rendered_page(string path, int page, float zoom_level, int* page_width, int* page_height) {
	//fz_document* doc = get_document_with_path(path);
	if (path.size() > 0) {
		RenderRequest req;
		req.path = path;
		req.page = page;
		req.zoom_level = zoom_level;
		cached_response_mutex.lock();
		GLuint result = 0;
		for (auto& cached_resp : cached_responses) {
			if (cached_resp.request == req) {
				cached_resp.last_access_time = SDL_GetTicks();

				if (page_width) *page_width = cached_resp.pixmap->w;
				if (page_height) *page_height = cached_resp.pixmap->h;

				// We can only use OpenGL in the main thread, so we can not upload the rendered
				// pixmap into a texture in the worker thread, so whenever we get a rendered page
				// in the main thread, we initialize its OpenGL texture if it is not initialized already
				if (cached_resp.texture != 0) {
					result = cached_resp.texture;
				}
				else {
					glGenTextures(1, &result);
					glBindTexture(GL_TEXTURE_2D, result);

					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);


					// OpenGL usually expects powers of two textures and since our pixmaps dimensions are
					// often not powers of two, we set the unpack alignment to 1 (no alignment) 
					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
					glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
					glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cached_resp.pixmap->w, cached_resp.pixmap->h, 0, GL_RGB, GL_UNSIGNED_BYTE, cached_resp.pixmap->samples);
					cout << "texture: " << result << endl;
					cached_resp.texture = result;
				}
				break;
			}
		}
		cached_response_mutex.unlock();
		if (result == 0) {
			add_request(path, page, zoom_level);
			return try_closest_rendered_page(path, page, zoom_level, page_width, page_height);
		}
		return result;
	}
	return 0;
}

GLuint PdfRenderer::try_closest_rendered_page(string doc_path, int page, float zoom_level, int* page_width, int* page_height) {
	/*
	If the requested page is not available, we try to find the rendered page with the closest
	possible zoom level to our request and return that instead
	*/
	cached_response_mutex.lock();

	float min_diff = 10000.0f;
	GLuint best_texture = 0;

	for (const auto& cached_resp : cached_responses) {
		if ((cached_resp.request.path == doc_path) && (cached_resp.request.page == page) && (cached_resp.texture != 0)) {
			float diff = cached_resp.request.zoom_level - zoom_level;
			if (diff < min_diff) {
				min_diff = diff;
				best_texture = cached_resp.texture;
				if (page_width) *page_width = static_cast<int>(cached_resp.pixmap->w * zoom_level / cached_resp.request.zoom_level);
				if (page_height) *page_height = static_cast<int>(cached_resp.pixmap->h * zoom_level / cached_resp.request.zoom_level);
			}
		}
	}
	cached_response_mutex.unlock();
	return best_texture;
}

void PdfRenderer::delete_old_pages() {
	/*
	Deletes old cached pages. This function should only be called from the main thread.
	OpenGL textures are released immediately but pixmaps should be freed from the thread
	that created them, so we add old pixmaps to pixmaps_to_delete and delete them later from the worker thread
	*/
	cached_response_mutex.lock();
	vector<int> indices_to_delete;
	unsigned int now = SDL_GetTicks();

	for (int i = 0; i < cached_responses.size(); i++) {
		if ((now - cached_responses[i].last_access_time) > cache_invalid_milies) {
			cout << "deleting cached texture ... " << endl;
			indices_to_delete.push_back(i);
		}
	}

	// We erase from back to front so that erasing one element does not change
	// the index of other elements
	for (int j = indices_to_delete.size() - 1; j >= 0; j--) {
		int index_to_delete = indices_to_delete[j];
		RenderResponse resp = cached_responses[index_to_delete];

		pixmap_drop_mutex.lock();
		pixmaps_to_drop.push_back(resp.pixmap);
		pixmap_drop_mutex.unlock();

		if (resp.texture != 0) {
			glDeleteTextures(1, &resp.texture);
		}

		cached_responses.erase(cached_responses.begin() + index_to_delete);
	}
	cached_response_mutex.unlock();
}

PdfRenderer::~PdfRenderer() {
}

fz_document* PdfRenderer::get_document_with_path(string path) {
	if (opened_documents.find(path) != opened_documents.end()) {
		return opened_documents.at(path);
	}
	fz_document* ret_val = nullptr;
	fz_try(mupdf_context) {
		ret_val = fz_open_document(mupdf_context, path.c_str());
		opened_documents[path] = ret_val;
	}
	fz_catch(mupdf_context) {
		cout << "Error: could not open document" << endl;
	}

	return ret_val;
}

void PdfRenderer::delete_old_pixmaps() {
	// this function should only be called from the worker thread
	pixmap_drop_mutex.lock();
	for (int i = 0; i < pixmaps_to_drop.size(); i++) {
		fz_try(mupdf_context) {
			fz_drop_pixmap(mupdf_context, pixmaps_to_drop[i]);
		}
		fz_catch(mupdf_context) {
			cout << "Error: could not drop pixmap" << endl;
		}
	}
	pixmaps_to_drop.clear();
	pixmap_drop_mutex.unlock();
}

void PdfRenderer::run(bool* should_quit) {
	init_context();

	while (!(*should_quit)) {
		pending_requests_mutex.lock();

		while (pending_requests.size() == 0) {
			pending_requests_mutex.unlock();
			delete_old_pixmaps();
			if (*should_quit) break;

			Sleep(100);
			pending_requests_mutex.lock();
		}
		if (*should_quit) break;
		cout << "worker thread running ... pending requests: " << pending_requests.size() << endl;

		RenderRequest req = pending_requests[pending_requests.size() - 1];

		if (req.type == REQUEST_RENDER) {
			// if the request is already rendered, just return the previous result
			cached_response_mutex.lock();
			bool is_already_rendered = false;
			for (const auto& cached_rep : cached_responses) {
				if (cached_rep.request == req) is_already_rendered = true;
			}
			cached_response_mutex.unlock();
			pending_requests.pop_back();
			pending_requests_mutex.unlock();

			if (!is_already_rendered) {

				fz_try(mupdf_context) {
					fz_matrix transform_matrix = fz_pre_scale(fz_identity, req.zoom_level, req.zoom_level);
					fz_document* doc = get_document_with_path(req.path);
					fz_pixmap* rendered_pixmap = fz_new_pixmap_from_page_number(mupdf_context, doc, req.page, transform_matrix, fz_device_rgb(mupdf_context), 0);
					RenderResponse resp;
					resp.request = req;
					resp.last_access_time = SDL_GetTicks();
					resp.pixmap = rendered_pixmap;
					resp.texture = 0;

					cached_response_mutex.lock();
					cached_responses.push_back(resp);
					cached_response_mutex.unlock();
					if (invalidate_pointer != nullptr) {
						//todo: there might be a race condition here
						*invalidate_pointer = true;
					}

				}
				fz_catch(mupdf_context) {
					cout << "Error: could not render page" << endl;
				}
			}
		}
		else if (req.type == REQUEST_SEARCH){
			pending_requests.pop_back();
			pending_requests_mutex.unlock();
			fz_document* doc = get_document_with_path(req.path);

			int num_pages = fz_count_pages(mupdf_context, doc);

			int total_results = 0;


			int num_handled_pages = 0;
			int i = req.page;
			while(num_handled_pages < num_pages){
				num_handled_pages++;
				fz_page* page = fz_load_page(mupdf_context, doc, i);

				const int max_hits_per_page = 20;
				fz_quad hitboxes[max_hits_per_page];
				int num_results = fz_search_page(mupdf_context, page, req.search_term.c_str(), hitboxes, max_hits_per_page);

				for (int j = 0; j < num_results; j++) {
					req.search_results_mutex->lock();
					req.search_results->push_back(SearchResult{ fz_rect_from_quad(hitboxes[j]), i });
					req.search_results_mutex->unlock();
					*invalidate_pointer = true;
				}
				if (i % 10 == 0) {
					req.search_results_mutex->lock();
					*req.percent_done = (float)num_handled_pages / num_pages;
					*invalidate_pointer = true;
					req.search_results_mutex->unlock();
				}

				total_results += num_results;

				fz_drop_page(mupdf_context, page);

				i = (i + 1) % num_pages;
			}
			req.search_results_mutex->lock();
			*req.is_searching = false;
			*invalidate_pointer = true;
			req.search_results_mutex->unlock();
		}

	}
}
