#ifndef CA_I18N_H
#define CA_I18N_H

#include <lua.h>
#include <stdbool.h>

const char* TR(const char* key);
void TR_add(const char* key, const char* value);
void load_locale(const char* name, bool verbose);
void init_i18n();
void i18n_register_lua(lua_State* L);

#define TR_REG(a) T.a = TR(#a)

#define TR_FIELDS(X)                  \
  /* generic */                       \
  X(ca)                               \
  X(confirm)                          \
  X(want_to_save_changes)             \
  X(dont_save)                        \
  X(save)                             \
  X(cancel)                           \
  X(close)                            \
  X(delete)                           \
  X(unsubscribe)                      \
  /* main */                          \
  X(main_new_leg)                     \
  X(main_open_leg)                    \
  X(main_save_leg)                    \
  X(main_saveas_leg)                  \
  X(main_open_blueprint_leg)          \
  X(main_exit_leg)                    \
  X(main_layer_push_leg)              \
  X(main_layer_pop_leg)               \
  X(main_layer_show_leg)              \
  X(main_layer_f1_leg)                \
  X(main_layer_f2_leg)                \
  X(main_layer_f3_leg)                \
  X(main_select_level)                \
  X(main_select_level_leg)            \
  X(main_select_level_custom)         \
  X(main_select_level_custom_leg)     \
  X(main_wiki_leg)                    \
  X(main_settings_leg)                \
  X(main_open_sel_leg)                \
  X(main_save_sel_leg)                \
  X(main_bp_add_leg)                  \
  X(main_speed1_leg)                  \
  X(main_speed2_leg)                  \
  X(main_speed3_leg)                  \
  X(main_speed4_leg)                  \
  X(main_speed5_leg)                  \
  X(main_speed6_leg)                  \
  X(main_show_start_leg)              \
  X(main_simu_stop_leg)               \
  X(main_simu_start_leg)              \
  X(main_simu_err_leg)                \
  X(main_pause_leg)                   \
  X(main_rewind_disabled_leg)         \
  X(main_rewind_leg)                  \
  X(main_forward_leg)                 \
  X(main_brush_leg)                   \
  X(main_line_leg)                    \
  X(main_bucket_leg)                  \
  X(main_picker_leg)                  \
  X(main_marquee_leg)                 \
  X(main_text_leg)                    \
  X(main_fliph_leg)                   \
  X(main_flipv_leg)                   \
  X(main_rot_leg)                     \
  X(main_fill_leg)                    \
  X(main_linesep_leg)                 \
  X(main_linesep_reset_leg)           \
  X(main_btn_wiki)                    \
  X(main_bar_level)                   \
  X(main_bar_img)                     \
  X(main_bar_img_name)                \
  X(main_bar_tick)                    \
  X(main_bar_energy)                  \
  X(main_bar_zoom)                    \
  X(main_bar_sel)                     \
  X(main_bar_coord)                   \
  X(main_cursor_sep)                  \
  X(main_cursor_width)                \
  X(main_msg_error)                   \
  X(main_could_not_save_sel)          \
  X(main_sel_saved)                   \
  X(main_could_not_open_image)        \
  X(main_more_than_one_nand)          \
  X(main_wire_too_slow)               \
  X(main_inventory_full)              \
  X(main_image_saved)                 \
  X(main_could_not_save_image)        \
  X(main_level_error)                 \
  X(main_separation_width)            \
  X(main_save_as_blueprint)           \
  X(main_insert_text)                 \
  X(main_insert_number)               \
  X(main_btn_blueprint)               \
  X(main_btn_blueprint_leg)           \
  X(main_soladd_leg)                  \
  X(main_blueprint_associated)        \
  X(main_blueprint_not_associated)    \
  X(main_campaign_loaded)             \
  X(main_file_level_loaded)           \
  X(main_custom_level_loaded)         \
  X(main_blueprint_saved)             \
  X(main_blueprint_marked)            \
  X(main_blueprint_linked)            \
  X(main_blueprint_unlinked)          \
  X(main_untitled)                    \
  /* Campaign */                      \
  X(select_campaign)                  \
  /* About */                         \
  X(about_photosensitivity_title)     \
  X(about_photosensitivity_text)      \
  X(about_title)                      \
  X(about_content)                    \
  /* Settings */                      \
  X(settings_about_leg)               \
  X(settings_header_general)          \
  X(settings_always_on_top)           \
  X(settings_header_drawing)          \
  X(settings_drawing_sound)           \
  X(settings_header_simulation)       \
  X(settings_wire_glow)               \
  X(settings_nand_sound)              \
  X(settings_title)                   \
  X(settings_on)                      \
  /* Settings */                      \
  X(wiki_title)                       \
  /* Blueprint */                     \
  X(bp_couldnt_create_inventory_file) \
  X(bp_error_writing_inventory)       \
  X(bp_pages)                         \
  X(bp_solutions)                     \
  X(bp_inventory)                     \
  X(bp_fixed_slots)                   \
  X(bp_delete_blueprint_confirm)      \
  X(bp_unsubscribe_confirm)           \
  X(bp_pages_editing)                 \
  X(bp_title)                         \
  X(bp_delete)                        \
  X(bp_rename)                        \
  X(bp_use)                           \
  X(bp_rot)                           \
  X(bp_publish)                       \
  X(bp_edit_page_icons_leg)           \
  X(bp_use_leg)                       \
  X(bp_rot_leg)                       \
  X(bp_del_leg)                       \
  X(bp_rename_leg)                    \
  X(bp_details)                       \
  X(bp_blueprint_deleted)             \
  X(bp_cant_delete)                   \
  X(bp_detail_leg)                    \
  X(bp_workshop_leg)                  \
  /* BPDetail */                      \
  X(bpdetail_title)                   \
  X(bpdetail_unnamed)                 \
  X(bpdetail_paste)                   \
  X(bpdetail_paste_leg)               \
  X(bpdetail_loadwlevel)              \
  X(bpdetail_loadwlevel_leg)          \
  X(bpdetail_rot_leg)                 \
  X(bpdetail_copy_leg)                \
  X(bpdetail_delete_leg)              \
  X(bpdetail_edit_leg)                \
  X(bpdetail_load_leg)                \
  X(bpdetail_rename_leg)              \
  X(bpdetail_publish_leg)             \
  X(bpdetail_steam_leg)               \
  /* Pubform */                       \
  X(pubform_publishing)               \
  X(pubform_uploading)                \
  X(pubform_legal)                    \
  X(pubform_label_title)              \
  X(pubform_label_desc)               \
  X(pubform_title)                    \
  X(pubform_submit)                   \
  /* Levels */                        \
  X(levels_title)                     \
  X(levels_submit)                    \
  X(levels_publish)                   \
  X(levels_unsubscribe)               \
  X(levels_wiki)                      \
  X(levels_change_campaign)           \
  /* Custom Level */                  \
  X(customlvl_title)                  \
  X(customlvl_workshop_btn)           \
  X(customlvl_subscribed)             \
  X(customlvl_official)               \
  X(customlvl_local)                  \
  X(customlvl_prefix_local)           \
  X(customlvl_prefix_workshop)        \
  X(customlvl_prefix_official)        \
  X(customlvl_prefix_unknown)         \
  X(customlvl_unsubscribe_confirm)    \
  X(customlvl_wiki_leg)               \
  X(customlvl_official_leg)           \
  X(customlvl_local_leg)              \
  X(customlvl_browse_leg)             \
  X(customlvl_open_steam_leg)         \
  X(customlvl_publish_leg)            \
  X(customlvl_workshop_leg)           \
  X(customlvl_file_leg)               \
  X(customlvl_localfolder_leg)        \
  X(customlvl_levelfolder_leg)        \
  /* Workshop */                      \
  X(workshop_title)                   \
  X(workshop_title_solutions)         \
  X(workshop_sort_trending)           \
  X(workshop_sort_recent)             \
  X(workshop_sort_votes)              \
  X(workshop_browse_steam)            \
  X(workshop_subscribed)              \
  X(workshop_my_uploads)              \
  X(workshop_search)                  \
  X(workshop_fetching)                \
  X(workshop_no_results)              \
  X(workshop_subscribed_overlay)      \
  /* WorkshopDet */                   \
  X(workshopdet_subscribe)            \
  /* Sol Widget*/                     \
  X(sol_browse)                       \
  /* MText */                         \
  X(mtext_title)                      \
  X(mtext_accept)                     \
  /* Simu*/                           \
  X(simu_multiple_nands)              \
  X(simu_nand_missing_connection)     \
  X(simu_long_wire)                   \
  /* Paint */                         \
  X(paint_created_layer)              \
  X(paint_drag_resize)                \
  /* Generic */                       \
  X(by_author)

/* END*/

extern struct tr_s {
#define X(x) const char* x;
  TR_FIELDS(X)
#undef X
} T;

#endif

