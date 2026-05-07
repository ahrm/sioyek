#pragma once

#include <iostream>
#include <vector>
#include <iostream>
#include <string>
#include "sqlite3.h"
#include "book.h"

class CachedChecksummer;

struct MarkInDatabase {
    std::string checksum;
    char symbol;
    float offset_y;
    std::optional<float> offset_x;
    std::optional<float> zoom_level;
    std::wstring uuid;
};

class DatabaseManager {
private:
    sqlite3* local_db;
    sqlite3* global_db;
    bool create_opened_books_table();
    bool create_marks_table();
    bool create_bookmarks_table();
    bool create_links_table();
    void create_tables();
    bool create_document_hash_table();
    bool create_highlights_table();
    bool create_study_objects_table();
    bool create_problem_states_table();
    bool create_dependency_links_table();
    bool create_shelf_items_table();
    bool create_region_highlights_table();
public:
    bool open(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path);
    bool select_opened_book(const std::string& book_path, std::vector<OpenedBookState>& out_result);
    bool insert_mark(const std::string& checksum,
        char symbol,
        float offset_y,
        std::wstring uuid,
        std::optional<float> offset_x = {},
        std::optional<float> zoom_level = {});
    bool update_mark(const std::string& checksum, char symbol, float offset_y, std::optional<float> offset_x, std::optional<float> zoom_level);
    bool update_book(
        const std::string& path,
        float zoom_level,
        float offset_x,
        float offset_y,
        std::wstring actual_name=L"",
        bool two_page_mode=false,
        bool book_mode_cover_offset=false);
    bool select_mark(const std::string& checksum, std::vector<Mark>& out_result);
    bool insert_bookmark(const std::string& checksum, const std::wstring& desc, float offset_y, std::wstring uuid);
    bool insert_bookmark_marked(const std::string& checksum, const std::wstring& desc, float offset_x, float offset_y, std::wstring uuid);
    //bool insert_bookmark_freetext(const std::string& checksum, const std::wstring& desc, float begin_x, float begin_y, float end_x, float end_y, float color_red, float color_green, float color_blue, float font_size, std::string font_face, std::wstring uuid);
    bool insert_bookmark_freetext(const std::string& checksum, const BookMark& bm);
    bool select_bookmark(const std::string& checksum, std::vector<BookMark>& out_result);
    bool insert_portal(const std::string& src_checksum,
        const std::string& dst_checksum,
        float dst_offset_y,
        float dst_offset_x,
        float dst_zoom_level,
        float src_offset_y,
        std::wstring uuid);

    bool insert_visible_portal(const std::string& src_checksum,
        const std::string& dst_checksum,
        float dst_offset_y,
        float dst_offset_x,
        float dst_zoom_level,
        float src_offet_x,
        float src_offset_y,
        std::wstring uuid);

    bool select_links(const std::string& src_checksum, std::vector<Portal>& out_result);
    bool delete_portal(const std::string& uuid);
    bool delete_bookmark(const std::string& uuid);
    bool global_select_bookmark(std::vector<std::pair<std::string, BookMark>>& out_result);
    bool global_select_highlight(std::vector<std::pair<std::string, Highlight>>& out_result);
    //bool update_portal(const std::string& checksum, float dst_offset_x, float dst_offset_y, float dst_zoom_level, float src_offset_y);
    bool update_portal(const std::string& uuid, float dst_offset_x, float dst_offset_y, float dst_zoom_level);
    bool update_highlight_add_annotation(const std::string& uuid, const std::wstring& text_annot);
    bool update_highlight_type(const std::string& uuid, char new_type);
    bool update_bookmark_change_text(const std::string& uuid, const std::wstring& new_text, float new_font_size);
    bool update_bookmark_change_position(const std::string& uuid, AbsoluteDocumentPos new_begin, AbsoluteDocumentPos new_end);
    bool update_portal_change_src_position(const std::string& uuid, AbsoluteDocumentPos new_pos);
    bool select_opened_books_path_values(std::vector<std::wstring>& out_result);
    bool select_opened_books_path_and_doc_names(std::vector<std::pair<std::wstring, std::wstring>>& out_result);
    bool delete_mark_with_symbol(char symbol);
    bool delete_mark_with_uuid(const std::string& uuid);
    bool select_global_mark(char symbol, std::vector<std::pair<std::string, float>>& out_result);
    bool delete_opened_book(const std::string& book_path);
    bool delete_highlight(const std::string& uuid);
    bool select_highlight(const std::string& checksum, std::vector<Highlight>& out_result);
    bool select_highlight_with_type(const std::string& checksum, char type, std::vector<Highlight>& out_result);
    bool set_actual_document_name(const std::string& checksum, const std::wstring& actual_name);
    bool insert_highlight(const std::string& checksum,
        const std::wstring& desc,
        float begin_x,
        float begin_y,
        float end_x,
        float end_y,
        char type,
        std::wstring uuid);
    bool insert_highlight_with_annotation(const std::string& checksum,
        const std::wstring& desc,
        const std::wstring& annot,
        float begin_x,
        float begin_y,
        float end_x,
        float end_y,
        char type,
        std::wstring uuid);
    bool insert_study_object(const StudyObject& study_object);
    bool select_study_objects(const std::string& checksum, std::vector<StudyObject>& out_result);
    bool select_study_objects_for_checksums(const std::vector<std::string>& checksums, std::vector<StudyObject>& out_result);
    bool select_study_objects_for_documents(const std::vector<std::string>& checksums, const std::vector<std::wstring>& document_paths, std::vector<StudyObject>& out_result);
    bool select_study_objects_by_ids(const std::vector<std::string>& ids, std::vector<StudyObject>& out_result);
    bool update_study_object_title(const std::string& id, const std::wstring& new_title);
    bool update_study_object_type(const std::string& id, const std::wstring& new_type);
    bool delete_study_object(const std::string& id);
    bool insert_study_link(const StudyLink& study_link);
    bool select_study_links_for_study_objects(const std::vector<std::string>& study_object_ids, std::vector<StudyLink>& out_result);
    bool select_study_links_for_study_object(const std::string& study_object_id, std::vector<StudyLink>& out_result);
    bool update_study_link_type(const std::string& id, const std::wstring& relation_type);
    bool update_study_link_note(const std::string& id, const std::wstring& note);
    bool delete_study_link(const std::string& id);
    bool insert_problem_state(const ProblemState& problem_state);
    bool upsert_problem_status(const std::string& study_object_id, const std::wstring& status);
    bool update_problem_solution_ref(const std::string& study_object_id, const std::wstring& solution_ref);
    bool clear_problem_solution_ref(const std::string& study_object_id);
    bool select_problem_states_for_study_objects(const std::vector<std::string>& study_object_ids, std::vector<ProblemState>& out_result);
    bool insert_shelf_item(const ShelfItem& shelf_item);
    bool select_shelf_items(std::vector<ShelfItem>& out_result);
    bool delete_shelf_item(const std::string& id);
    bool clear_shelf_items();
    bool update_shelf_item_order(const std::string& id, int item_order);
    bool insert_region_highlight(const RegionHighlight& region_highlight);
    bool select_region_highlights(const std::string& checksum, std::vector<RegionHighlight>& out_result);
    bool select_region_highlights_for_documents(const std::vector<std::string>& checksums, const std::vector<std::wstring>& document_paths, std::vector<RegionHighlight>& out_result);
    bool update_region_highlight_title(const std::string& id, const std::wstring& new_title);
    bool delete_region_highlight(const std::string& id);
    bool get_path_from_hash(const std::string& checksum, std::vector<std::wstring>& out_paths);
    bool get_hash_from_path(const std::string& path, std::vector<std::wstring>& out_checksum);
    bool get_prev_path_hash_pairs(std::vector<std::pair<std::wstring, std::wstring>>& out_pairs);
    bool insert_document_hash(const std::wstring& path, const std::string& checksum);
    void upgrade_database_hashes();
    void split_database(const std::wstring& local_database_path, const std::wstring& global_database_path, bool was_using_hashes);
    void export_json(std::wstring json_file_path, CachedChecksummer* checksummer);
    void import_json(std::wstring json_file_path, CachedChecksummer* checksummer);
    void ensure_database_compatibility(const std::wstring& local_db_file_path, const std::wstring& global_db_file_path);
    void ensure_schema_compatibility();
    int get_version();
    int set_version();
    bool run_schema_query(const char* query);
    void migrate_version_0_to_1();
    void migrate_version_1_to_2();
    void migrate_version_2_to_3();
    void migrate_version_3_to_4();
    void migrate_version_4_to_5();
    void migrate_version_5_to_6();
    void migrate_version_6_to_7();
    void migrate_version_7_to_8();
    bool select_all_mark_ids(std::vector<int>& mark_ids);
    bool select_all_marks(std::vector<MarkInDatabase>& marks);
    bool select_all_bookmark_ids(std::vector<int>& mark_ids);
    bool select_all_highlight_ids(std::vector<int>& mark_ids);
    bool select_all_portal_ids(std::vector<int>& mark_ids);

    std::string get_annot_table_name(Annotation* annot);

    bool insert_annotation(Annotation* annot, std::string document_hash);
    bool update_annotation(Annotation* annot);
    bool delete_annotation(Annotation* annot);
    bool update_file_name(std::wstring old_name, std::wstring new_name);

    std::wstring generic_update_create_query(std::string table_name,
        std::vector<std::pair<std::string, QVariant>> selections,
        std::vector<std::pair<std::string, QVariant>> updated_values);

    std::wstring generic_insert_create_query(std::string table_name,
        std::vector<std::pair<std::string, QVariant>> values);

    bool generic_update_run_query(std::string table_name,
        std::vector<std::pair<std::string, QVariant>> selections,
        std::vector<std::pair<std::string, QVariant>> updated_values);

    bool generic_insert_run_query(std::string table_name,
        std::vector<std::pair<std::string, QVariant>> values);
};
