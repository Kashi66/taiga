/*
** Taiga
** Copyright (C) 2010-2018, Eren Okka
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "base/file.h"
#include "base/format.h"
#include "base/string.h"
#include "base/time.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "library/export.h"
#include "sync/myanimelist_types.h"
#include "sync/myanimelist_util.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"

namespace library {

bool ExportAsMalXml(const std::wstring& path) {
  constexpr auto count_total_anime = []() {
    int count = 0;
    for (const auto& [id, item] : AnimeDatabase.items) {
      if (item.IsInList()) {
        count += 1;
      }
    }
    return count;
  };

  constexpr auto tr_series_type = [](int type) {
    switch (sync::myanimelist::TranslateSeriesTypeTo(type)) {
      default:
      case sync::myanimelist::kUnknownType: return L"Unknown";
      case sync::myanimelist::kTv: return L"TV";
      case sync::myanimelist::kOva: return L"OVA";
      case sync::myanimelist::kMovie: return L"Movie";
      case sync::myanimelist::kSpecial: return L"Special";
      case sync::myanimelist::kOna: return L"ONA";
      case sync::myanimelist::kMusic: return L"Music";
    }
  };

  constexpr auto tr_my_status = [](int status) {
    switch (sync::myanimelist::TranslateMyStatusTo(status)) {
      default:
      case sync::myanimelist::kWatching: return L"Watching";
      case sync::myanimelist::kCompleted: return L"Completed";
      case sync::myanimelist::kOnHold: return L"On-Hold";
      case sync::myanimelist::kDropped: return L"Dropped";
      case sync::myanimelist::kPlanToWatch: return L"Plan to Watch";
    }
  };

  xml_document document;

  xml_node node_decl = document.prepend_child(pugi::node_declaration);
  node_decl.append_attribute(L"version") = L"1.0";
  node_decl.append_attribute(L"encoding") = L"UTF-8";

  xml_node node_comment = document.append_child(pugi::node_comment);
  node_comment.set_value(L" Generated by Taiga v{} on {} {} "_format(
      Taiga.version.to_string(), GetDate().to_string(), GetTime()).c_str());

  xml_node node_myanimelist = document.append_child(L"myanimelist");

  xml_node node_myinfo = node_myanimelist.append_child(L"myinfo");
  XmlWriteIntValue(node_myinfo, L"user_id", 0);
  XmlWriteStrValue(node_myinfo, L"user_name", taiga::GetCurrentUsername().c_str());
  XmlWriteIntValue(node_myinfo, L"user_export_type", 1);  // anime
  XmlWriteIntValue(node_myinfo, L"user_total_anime", count_total_anime());
  XmlWriteIntValue(node_myinfo, L"user_total_watching", AnimeDatabase.GetItemCount(anime::kWatching));
  XmlWriteIntValue(node_myinfo, L"user_total_completed", AnimeDatabase.GetItemCount(anime::kCompleted));
  XmlWriteIntValue(node_myinfo, L"user_total_onhold", AnimeDatabase.GetItemCount(anime::kOnHold));
  XmlWriteIntValue(node_myinfo, L"user_total_dropped", AnimeDatabase.GetItemCount(anime::kDropped));
  XmlWriteIntValue(node_myinfo, L"user_total_plantowatch", AnimeDatabase.GetItemCount(anime::kPlanToWatch));

  for (const auto&[id, item] : AnimeDatabase.items) {
    if (item.IsInList()) {
      xml_node node = node_myanimelist.append_child(L"anime");
      XmlWriteIntValue(node, L"series_animedb_id", item.GetId());
      XmlWriteStrValue(node, L"series_title", item.GetTitle().c_str(), pugi::node_cdata);
      XmlWriteStrValue(node, L"series_type", tr_series_type(item.GetType()));
      XmlWriteIntValue(node, L"series_episodes", item.GetEpisodeCount());

      XmlWriteIntValue(node, L"my_id", 0);
      XmlWriteIntValue(node, L"my_watched_episodes", item.GetMyLastWatchedEpisode());
      XmlWriteStrValue(node, L"my_start_date", item.GetMyDateStart().to_string().c_str());
      XmlWriteStrValue(node, L"my_finish_date", item.GetMyDateEnd().to_string().c_str());
      XmlWriteStrValue(node, L"my_fansub_group", L"", pugi::node_cdata);
      XmlWriteStrValue(node, L"my_rated", L"");
      XmlWriteIntValue(node, L"my_score", sync::myanimelist::TranslateMyRatingTo(item.GetMyScore()));
      XmlWriteStrValue(node, L"my_dvd", L"");
      XmlWriteStrValue(node, L"my_storage", L"");
      XmlWriteStrValue(node, L"my_status", tr_my_status(item.GetMyStatus()));
      XmlWriteStrValue(node, L"my_comments", item.GetMyNotes().c_str(), pugi::node_cdata);
      XmlWriteIntValue(node, L"my_times_watched", item.GetMyRewatchedTimes());
      XmlWriteStrValue(node, L"my_rewatch_value", L"");
      XmlWriteIntValue(node, L"my_downloaded_eps", 0);
      XmlWriteStrValue(node, L"my_tags", item.GetMyTags().c_str(), pugi::node_cdata);
      XmlWriteIntValue(node, L"my_rewatching", item.GetMyRewatching());
      XmlWriteIntValue(node, L"my_rewatching_ep", item.GetMyRewatchingEp());
      XmlWriteIntValue(node, L"update_on_import", 0);
    }
  }

  CreateFolder(GetPathOnly(path));
  return document.save_file(path.c_str(), L"\t",
                            pugi::format_default,
                            pugi::xml_encoding::encoding_utf8);
}

}  // namespace library
