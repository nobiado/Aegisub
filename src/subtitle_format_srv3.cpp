// Copyright (c) 2022, Adriano Dal Bosco
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of the Aegisub Group nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Aegisub Project http://www.aegisub.org/

/// @file subtitle_format_srv3.cpp
/// @brief Support for export to YouTube internal undocumented format srv3 (xml)
/// @ingroup subtitle_io

#include "ass_file.h"
#include "ass_dialogue.h"
#include "text_file_writer.h"

#include <libaegisub/format.h>

#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/home/classic/tree/tree_to_xml.hpp>

#include "subtitle_format_srv3.h"


namespace format {
    enum penflag {
        clear = 0,
        bold =  2,
        italic = 4,
        underlined = 8
    };
}


Srv3SubtitleFormat::Srv3SubtitleFormat()
: SubtitleFormat("YouTube SRV3")
{
}


std::vector<std::string> Srv3SubtitleFormat::GetWriteWildcards() const {
    return {"srv3"};
}


void Srv3SubtitleFormat::WriteFile(const AssFile *src, agi::fs::path const& filename, agi::vfr::Framerate const& fps, std::string const&) const {
    // Convert to encore
    AssFile copy(*src);
    copy.Sort();
    StripComments(copy);
    RecombineOverlaps(copy);
    MergeIdentical(copy);

    TextFileWriter file(filename, "UTF-8");
    // Maybe declare these as flyweights?  Not sure.
    using Cue = std::string;
    using Pen = std::string;
    using Wp  = std::string;
    std::vector<Cue> cues;
    std::vector<Pen> pens;
    std::vector<Wp>  wss;
    format::penflag pflag;


    file.WriteLineToFile("<?xml version=\"1.0\" encoding=\"utf-8\" ?><timedtext format=\"3\">");
    file.WriteLineToFile("<head>\n</head>");

    std::string cue;

    for (auto const& current : copy.Events)
    {
        cue.clear();
        cue.append(agi::format("<p t=\"%i\" d=\"%i\">",
                     current.Start,
                     current.End-current.Start));

        for ( auto const& bl: current.ParseTags() )
        {
            switch (bl->GetType()) {
                case AssBlockType::PLAIN:
                {
                    std::string s{boost::spirit::classic::xml::encode(bl->GetText())};
                    for (auto pos = s.find("\\N"); pos != std::string::npos;pos = s.find("\\N"))
                    s.replace(pos,2,"\n");
                    cue.append(s);
                    break;
                }
                case AssBlockType::OVERRIDE:
                {
                    auto ovr = static_cast<AssDialogueBlockOverride*>(bl.get());
                    ovr->ParseTags();
                    for (auto& tag : ovr->Tags) {

                        file.WriteLineToFile("Override: ", false);
                        file.WriteLineToFile(tag.Name + ":", false);
                        for ( auto& tag : tag.Params ) {
                            file.WriteLineToFile(tag.Get<std::string>()+"\n", false);
                        }

                    }
                    break;
                }
                case AssBlockType::COMMENT:
                    break;

                case AssBlockType::DRAWING:
                    break;
            }
        }

    cue.append("</p>");
    cues.push_back(cue);

//        std::string s{boost::spirit::classic::xml::encode(current.Text.get())};
//        for (auto pos = s.find("\\N"); pos != std::string::npos;pos = s.find("\\N"))
//            s.replace(pos,2,"\n");
//        file.WriteLineToFile(agi::format("<p t=\"%i\" d=\"%i\">%s</p>",
//                                         current.Start,
//                                         current.End-current.Start,
//                                         s));
    }

    file.WriteLineToFile("<body>");
    for (auto&& cue: cues )
        file.WriteLineToFile(cue);
    file.WriteLineToFile("</body>");
    file.WriteLineToFile("</timedtext>");

    return;
}

