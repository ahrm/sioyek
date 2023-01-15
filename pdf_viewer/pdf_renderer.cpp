#include "pdf_renderer.h"
#include "utils.h"
#include <qdatetime.h>

extern bool LINEAR_TEXTURE_FILTERING;
//extern bool AUTO_EMBED_ANNOTATIONS;

PdfRenderer::PdfRenderer(int num_threads, bool* should_quit_pointer, fz_context* context_to_clone, float display_scale) : context_to_clone(context_to_clone),
pixmaps_to_drop(num_threads),
pixmap_drop_mutex(num_threads),
should_quit_pointer(should_quit_pointer),
num_threads(num_threads),
display_scale(display_scale)
{

	// this interval must be less than cache invalidation time
	garbage_collect_timer.setInterval(1000);
	garbage_collect_timer.start();

	QObject::connect(&garbage_collect_timer, &QTimer::timeout, [&]() {
			delete_old_pages();
		});
}

fz_context* PdfRenderer::init_context() {
	return fz_clone_context(context_to_clone);
}


void PdfRenderer::start_threads() {

	for (int i = 0; i < num_threads; i++) {
		worker_threads.push_back(std::thread([&, i]() {
				run(i);
			}));
	}
	search_thread = std::thread([&]() {
		run_search(num_threads);
		});
}

void PdfRenderer::join_threads()
{
	for (auto& worker : worker_threads) {
		worker.join();
	}
	search_thread.join();
}


void PdfRenderer::add_request(std::wstring document_path, int page, float zoom_level) {
	//fz_document* doc = get_document_with_path(document_path);
	if (document_path.size() > 0) {
		RenderRequest req;
		req.path = document_path;
		req.page = page;
		req.zoom_level = zoom_level;

		pending_requests_mutex.lock();
		bool should_add = true;
		for (size_t i = 0; i < pending_render_requests.size(); i++) {
			if (pending_render_requests[i] == req) {
				should_add = false;
			}
		}
		if (should_add) {
			pending_render_requests.push_back(req);
		}
		if (pending_render_requests.size() > (size_t) MAX_PENDING_REQUESTS) {
			pending_render_requests.erase(pending_render_requests.begin());
		}
		pending_requests_mutex.unlock();
	}
	else {
		std::wcout << "Error: could not find documnet" << std::endl;
	}
}
void PdfRenderer::add_request(std::wstring document_path,
	int page,
	std::wstring term,
	std::vector<SearchResult>* out,
	float* percent_done,
	bool* is_searching,
	std::mutex* mut,
	std::optional<std::pair<int, int>> range) {

	//fz_document* doc = get_document_with_path(document_path);
	if (document_path.size() > 0) {

		SearchRequest req;
		req.path = document_path;
		req.start_page = page;
		req.search_term = term;
		req.search_results_mutex = mut;
		req.search_results = out;
		req.percent_done = percent_done;
		req.is_searching = is_searching;
		req.range = range;

		search_request_mutex.lock();
		pending_search_request = req;
		search_request_mutex.unlock();
	}
	else {
		std::wcout << "Error: could not find document" << std::endl;
	}
}

//should only be called from the main thread

GLuint PdfRenderer::find_rendered_page(std::wstring path, int page, float zoom_level, int* page_width, int* page_height) {
	//fz_document* doc = get_document_with_path(path);
	if (path.size() > 0) {
		RenderRequest req;
		req.path = path;
		req.page = page;
		req.zoom_level = zoom_level;
		cached_response_mutex.lock();
		GLuint result = 0;
		for (auto& cached_resp : cached_responses) {
			if ((cached_resp.request == req) && (cached_resp.invalid == false)) {
				cached_resp.last_access_time = QDateTime::currentMSecsSinceEpoch();

				if (page_width) *page_width = cached_resp.width;
				if (page_height) *page_height = cached_resp.height;

				// We can only use OpenGL in the main thread, so we can not upload the rendered
				// pixmap into a texture in the worker thread, so whenever we get a rendered page
				// in the main thread, we initialize its OpenGL texture if it is not initialized already
				if (cached_resp.texture != 0) {
					result = cached_resp.texture;
				}
				else {
					glGenTextures(1, &result);
					glBindTexture(GL_TEXTURE_2D, result);

					if (LINEAR_TEXTURE_FILTERING) {
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
					}
					else {
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
					}

#ifdef GL_CLAMP
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#else
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#endif


					// OpenGL usually expects powers of two textures and since our pixmaps dimensions are
					// often not powers of two, we set the unpack alignment to 1 (no alignment) 

					glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cached_resp.pixmap->w, cached_resp.pixmap->h, 0, GL_RGB, GL_UNSIGNED_BYTE, cached_resp.pixmap->samples);
					glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

					// don't need the pixmap anymore
					pixmap_drop_mutex[cached_resp.thread].lock();
					pixmaps_to_drop[cached_resp.thread].push_back(cached_resp.pixmap);
					cached_resp.texture = result;
					pixmap_drop_mutex[cached_resp.thread].unlock();

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

GLuint PdfRenderer::try_closest_rendered_page(std::wstring doc_path, int page, float zoom_level, int* page_width, int* page_height) {
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
			if (diff <= min_diff) {
				min_diff = diff;
				best_texture = cached_resp.texture;
				if (page_width) *page_width = static_cast<int>(cached_resp.width * zoom_level / cached_resp.request.zoom_level);
				if (page_height) *page_height = static_cast<int>(cached_resp.height * zoom_level / cached_resp.request.zoom_level);
			}
		}
	}
	cached_response_mutex.unlock();
	return best_texture;
}

void PdfRenderer::delete_old_pages(bool force_all, bool invalidate_all) {
	/*
	Deletes old cached pages. This function should only be called from the main thread.
	OpenGL textures are released immediately but pixmaps should be freed from the thread
	that created them, so we add old pixmaps to pixmaps_to_delete and delete them later from the worker thread
	*/
	cached_response_mutex.lock();
	std::vector<int> indices_to_delete;
	unsigned int now = QDateTime::currentMSecsSinceEpoch();
	std::vector<int> cached_response_times;

	for (size_t i = 0; i < cached_responses.size(); i++) {
		cached_response_times.push_back(now - cached_responses[i].last_access_time);
	}
	int N = 5;

	if (invalidate_all) {
		for (size_t i = 0; i < cached_responses.size(); i++) {
			cached_responses[i].invalid = true;
		}
		are_documents_invalidated = true;
	}

	if (force_all) {
		for (size_t i = 0; i < cached_responses.size(); i++) {
			indices_to_delete.push_back(i);
		}
		are_documents_invalidated = true;
	}
	else if (cached_response_times.size() > (size_t) N) {
		// we never delete N most recent pages
		// todo: make this configurable
		std::nth_element(cached_response_times.begin(), cached_response_times.begin() + N - 1, cached_response_times.end());


		unsigned int time_threshold = now - cached_response_times[N - 1];

		for (size_t i = 0; i < cached_responses.size(); i++) {
			if ((cached_responses[i].last_access_time < time_threshold)
				&& ((now - cached_responses[i].last_access_time) > CACHE_INVALID_MILIES)) {
				indices_to_delete.push_back(i);
			}
		}
	}

	// We erase from back to front so that erasing one element does not change
	// the index of other elements
	for (int j = indices_to_delete.size() - 1; j >= 0; j--) {
		int index_to_delete = indices_to_delete[j];
		RenderResponse resp = cached_responses[index_to_delete];

		pixmap_drop_mutex[resp.thread].lock();
		if (resp.texture == 0) {
			pixmaps_to_drop[resp.thread].push_back(resp.pixmap);
		}
		pixmap_drop_mutex[resp.thread].unlock();

		if (resp.texture != 0) {
			glDeleteTextures(1, &resp.texture);
		}

		cached_responses.erase(cached_responses.begin() + index_to_delete);
	}
	cached_response_mutex.unlock();
}

void PdfRenderer::run_search(int thread_index)
{
	fz_context* mupdf_context  = init_context();

	while (!(*should_quit_pointer)) {
		if (pending_search_request.has_value()) {

			search_request_mutex.lock();
			SearchRequest req = pending_search_request.value();
			pending_search_request = {};
			search_request_mutex.unlock();

			fz_document* doc = get_document_with_path(thread_index, mupdf_context, req.path);

			int num_pages_in_document = fz_count_pages(mupdf_context, doc);

			int page_begin = 0;
			int page_end = num_pages_in_document-1;

			if (req.range) {
				page_begin = req.range.value().first-1;
				page_end = req.range.value().second-1;
			}
			// make sure page range is valid {
			if (page_begin < 0) page_begin = 0;
			if (page_begin > num_pages_in_document-1) page_begin = num_pages_in_document-1;
			if (page_end < 0) page_end = 0;
			if (page_end > num_pages_in_document-1) page_end = num_pages_in_document-1;
			//}

			int num_pages = page_end - page_begin + 1;

			req.search_results_mutex->lock();
			req.search_results->clear();
			*req.is_searching = true;
			req.search_results_mutex->unlock();

			if (req.start_page > page_end || req.start_page < page_begin) {
				req.start_page = page_begin;
			}

			int total_results = 0;
			int num_handled_pages = 0;
			int i = req.start_page;
			while (num_handled_pages < num_pages && (!pending_search_request.has_value()) && (!(*should_quit_pointer))) {
				num_handled_pages++;
				fz_page* page = fz_load_page(mupdf_context, doc, i);

				const int max_hits_per_page = 20;
				fz_quad hitboxes[max_hits_per_page];
                                int hit_mark[max_hits_per_page];
				int num_results = fz_search_page(mupdf_context, page, utf8_encode(req.search_term).c_str(), hit_mark, hitboxes, max_hits_per_page);

				if (num_results > 0) {
					req.search_results_mutex->lock();
					for (int j = 0; j < num_results; j++) {
						if (hit_mark[j] == 1) {
							// Hit box belongs to new entry
							req.search_results->push_back(SearchResult{ std::vector<fz_rect>(), i });
						}
						req.search_results->back().rects.push_back(fz_rect_from_quad(hitboxes[j]));
					}
					req.search_results_mutex->unlock();
				}

				if (num_handled_pages % 16 == 0) {
					*req.percent_done = (float)num_handled_pages / num_pages;
					//*invalidate_pointer = true;
					emit search_advance();
				}

				total_results += num_results;

				fz_drop_page(mupdf_context, page);

				i++;
				if (i > page_end) {
					i = page_begin;
				}
			}
			req.search_results_mutex->lock();
			*req.is_searching = false;
			//*invalidate_pointer = true;
			//if (on_search_invalidate) {
			//	on_search_invalidate();
			//}
			emit search_advance();
			req.search_results_mutex->unlock();
		}
		else{
			sleep_ms(100);
		}
	}
}

PdfRenderer::~PdfRenderer() {
}

fz_document* PdfRenderer::get_document_with_path(int thread_index, fz_context* mupdf_context, std::wstring path) {

	std::pair<int, std::wstring> document_id = std::make_pair(thread_index, path);

	if (opened_documents.find(document_id) != opened_documents.end()) {
		return opened_documents.at(document_id);
	}

	fz_document* ret_val = nullptr;
	fz_try(mupdf_context) {
		ret_val = fz_open_document(mupdf_context, utf8_encode(path).c_str());

		if (fz_needs_password(mupdf_context, ret_val)) {
			if (document_passwords.find(path) != document_passwords.end()) {
				fz_authenticate_password(mupdf_context, ret_val, document_passwords[path].c_str());
			}
		}

		opened_documents[make_pair(thread_index, path)] = ret_val;
	}
	fz_catch(mupdf_context) {
		std::wcout << "Error: could not open document" << std::endl;
	}

	return ret_val;
}

void PdfRenderer::delete_old_pixmaps(int thread_index, fz_context* mupdf_context) {
	// this function should only be called from the worker thread
	pixmap_drop_mutex[thread_index].lock();
	for (size_t i = 0; i < pixmaps_to_drop[thread_index].size(); i++) {
		fz_try(mupdf_context) {
			fz_drop_pixmap(mupdf_context, pixmaps_to_drop[thread_index][i]);
		}
		fz_catch(mupdf_context) {
			std::wcout << "Error: could not drop pixmap" << std::endl;
		}
	}
	pixmaps_to_drop[thread_index].clear();
	pixmap_drop_mutex[thread_index].unlock();
}
void PdfRenderer::clear_cache() {
	delete_old_pages(false, true);
}

void PdfRenderer::run(int thread_index) {
	fz_context* mupdf_context  = init_context();

	while (!(*should_quit_pointer)) {
		pending_requests_mutex.lock();

		while (pending_render_requests.size() == 0) {
			pending_requests_mutex.unlock();
			delete_old_pixmaps(thread_index, mupdf_context);
			if (*should_quit_pointer) break;

			sleep_ms(100);
			pending_requests_mutex.lock();
		}
		if (*should_quit_pointer) break;
		//cout << "worker thread running ... pending requests: " << pending_render_requests.size() << endl;

		RenderRequest req = pending_render_requests[pending_render_requests.size() - 1];

		// if the request is already rendered, just return the previous result
		cached_response_mutex.lock();

		if (are_documents_invalidated) {
			for (auto [_, document] : opened_documents) {
				fz_drop_document(mupdf_context, document);
			}
			opened_documents.clear();
			are_documents_invalidated = false;
		}

		bool is_already_rendered = false;
		for (const auto& cached_rep : cached_responses) {
			if ((cached_rep.request == req) && (cached_rep.invalid == false)) is_already_rendered = true;
		}
		cached_response_mutex.unlock();
		pending_render_requests.pop_back();
		pending_requests_mutex.unlock();

		if (!is_already_rendered) {

			fz_try(mupdf_context) {
				fz_matrix transform_matrix = fz_pre_scale(fz_identity, req.zoom_level * display_scale, req.zoom_level * display_scale);
				fz_document* doc = get_document_with_path(thread_index, mupdf_context, req.path);
				fz_pixmap* rendered_pixmap = nullptr;

				//if (AUTO_EMBED_ANNOTATIONS) {
				//	fz_page* page = fz_load_page(mupdf_context, doc, req.page);
				//	rendered_pixmap = fz_new_pixmap_from_page_contents(mupdf_context, page, transform_matrix, fz_device_rgb(mupdf_context), 0);
				//	fz_drop_page(mupdf_context, page);
				//}
				//else {
				rendered_pixmap = fz_new_pixmap_from_page_number(mupdf_context, doc, req.page, transform_matrix, fz_device_rgb(mupdf_context), 0);
				//}

				RenderResponse resp;
				resp.thread = thread_index;
				resp.request = req;
				resp.last_access_time = QDateTime::currentMSecsSinceEpoch();
				resp.pixmap = rendered_pixmap;
				resp.width = rendered_pixmap->w;
				resp.height = rendered_pixmap->h;
				resp.texture = 0;
				resp.invalid = false;

				cached_response_mutex.lock();
				cached_responses.push_back(resp);
				cached_response_mutex.unlock();

				emit render_advance();

			}
			fz_catch(mupdf_context) {
				std::cerr << "Error: could not render page" << std::endl;
			}
		}

	}
}

void PdfRenderer::add_password(std::wstring path, std::string password) {
	document_passwords[path] = password;
	delete_old_pages(true, false);
}

bool operator==(const RenderRequest& lhs, const RenderRequest& rhs) {
	if (rhs.path != lhs.path) {
		return false;
	}
	if (rhs.page != lhs.page) {
		return false;
	}
	if (rhs.zoom_level != lhs.zoom_level) {
		return false;
	}
	return true;
}

