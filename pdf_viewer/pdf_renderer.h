#pragma once

#include <vector>
#include <string>
#include <mupdf/fitz.h>
#include <mutex>
#include <variant>
#include <unordered_map>
#include <map>
#include <optional>
#include <iostream>
#include <functional>
#include <thread>
#include <unordered_map>
#include <map>
//#include <gl/glew.h>
//#include <Windows.h>

#include <qobject.h>
#include <qtimer.h>

#include "book.h"

extern const int MAX_PENDING_REQUESTS;
extern const unsigned int CACHE_INVALID_MILIES;

struct RenderRequest {
	std::wstring path;
	int page;
	float zoom_level;
};

struct SearchRequest {
	std::wstring path;
	int start_page;
	std::wstring search_term;
	std::vector<SearchResult>* search_results;
	std::mutex* search_results_mutex;
	float* percent_done = nullptr;
	bool* is_searching = nullptr;
	std::optional<std::pair<int, int>> range;
};

struct RenderResponse {
	RenderRequest request;
	unsigned int last_access_time;
	int thread;
	fz_pixmap* pixmap = nullptr;
	int width = -1;
	int height = -1;
	GLuint texture = 0;
	bool invalid = false;
};

bool operator==(const RenderRequest& lhs, const RenderRequest& rhs);

class PdfRenderer : public QObject{
	Q_OBJECT
	// A pointer to the mupdf context to clone.
	// Since the context should only be used from the thread that initialized it,
	// we can not simply clone the context in the initializer because the initializer
	// is called from the main thread. Instead, we just save a pointer to the context
	// in the initializer and then clone the context when run() is called in the worker
	//thread.
	fz_context* context_to_clone;

	std::vector<std::vector<fz_pixmap*>> pixmaps_to_drop;
	std::map<std::pair<int, std::wstring>, fz_document*> opened_documents;

	std::vector<RenderRequest> pending_render_requests;
	std::optional<SearchRequest> pending_search_request;
	std::vector<RenderResponse> cached_responses;
	std::vector<std::thread> worker_threads;
	std::thread search_thread;

	std::mutex pending_requests_mutex;
	std::mutex search_request_mutex;
	std::mutex cached_response_mutex;
	std::vector<std::mutex> pixmap_drop_mutex;

	QTimer garbage_collect_timer;

	bool* should_quit_pointer = nullptr;
	bool are_documents_invalidated = false;

	int num_threads = 0;
	float display_scale = 1.0f;

	std::map<std::wstring, std::string> document_passwords;

	fz_context* init_context();
	fz_document* get_document_with_path(int thread_index, fz_context* mupdf_context, std::wstring path);
	GLuint try_closest_rendered_page(std::wstring doc_path, int page, float zoom_level, int* page_width, int* page_height);
	void delete_old_pixmaps(int thread_index, fz_context* mupdf_context);
	void run(int thread_index);
	void run_search(int thread_index);

public:

	PdfRenderer(int num_threads, bool* should_quit_pointer, fz_context* context_to_clone, float display_scale);
	~PdfRenderer();
	void clear_cache();

	void start_threads();
	void join_threads();

	//should only be called from the main thread
	void add_request(std::wstring document_path, int page, float zoom_level);
	void add_request(std::wstring document_path,
		int page,
		std::wstring term,
		std::vector<SearchResult>* out,
		float* percent_done,
		bool* is_searching,
		std::mutex* mut,
		std::optional<std::pair<int,
		int>> range = {});

	GLuint find_rendered_page(std::wstring path, int page, float zoom_level, int* page_width, int* page_height);
	void delete_old_pages(bool force_all=false, bool invalidate_all=false);
	void add_password(std::wstring path, std::string password);

signals:
	void render_advance();
	void search_advance();

};
