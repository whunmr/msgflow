#include <sstream>
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <regex>
using namespace std;

namespace strings {
  static bool is_start_with(const string& s, const string& prefix) {
    return !s.compare(0, prefix.size(), prefix);
  }

  static string replace_all(const string& str, const string& from, const string& to) {
    string tmp = str;
    size_t start_pos = 0;
    while((start_pos = tmp.find(from, start_pos)) != string::npos) {
        tmp.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return tmp;
  }
  
  template <typename T>
  static string from_num(T num)
  {
     ostringstream ss;
     ss << num;
     return ss.str();
  }
}

static const char* k_mf_regex_prefix = "#!MF:regex:";
static const char* k_mf_reformat_to = "#!MF:reformat_to:";
static const char* k_mf_main_actor = "#!MF:main_actor:";
static const char* k_mf_draw_from_right = "#!MF:draw_from_right:";
static const char* k_mf_unknwn_msg_as_extra_info = "#!MF:unknwn_msg_as_extra_info:";
static const char* k_mf_spearator_line = "#!MF:separator_line:";
static const char* k_actor_span = "   ";
static const size_t k_max_info_extract_group = 16;

typedef map<string, size_t> ActorLineMap;
typedef list<string> Actors;

struct ExtractRule {
  string info_extract_regex_;  //.*\\[(\\w+)\\].*---->.*\\[(\\w+)\\] (.+?)  (.+?)  (.*)
  string reformat_to_;         //src:s1_, dst:s2_, msg_id:s3_, extra_info:s4_ s5_
};

struct MsgFlowOption {
  MsgFlowOption() : draw_from_right_(false), unknwn_msg_as_extra_info_(false) {/**/}
  
  string main_actor_;
  bool draw_from_right_;
  bool unknwn_msg_as_extra_info_;
  string separator_line_;
  vector<ExtractRule> extract_rules_;
};

struct DrawContext {
  DrawContext(const Actors actors) : actors_(actors) {
     header_line_ = fill_v_cols();
     template_line_ = compose_template_line();
  }

  string fill_v_cols() {
    string header_line;
    
    size_t actor_start_col = 0;
    for (Actors::const_iterator i = actors_.begin(); i != actors_.end(); ++i) {
      header_line += *i + k_actor_span;
      vcols_[*i] = actor_start_col + (i->length() / 2);
      actor_start_col += i->length() + strlen(k_actor_span);
    }
 
    return header_line + '\n';
  }

  string compose_template_line() {
    string template_line = string(header_line_.length(), ' ');
    for (map<string, size_t>::const_iterator i = vcols_.begin(); i != vcols_.end(); ++i) 
      template_line[i->second] = '|';
  
    return template_line;
  }

  int vcol_of(const string& actor) const {
     return vcols_[actor];
  }

  Actors actors_;

  mutable ActorLineMap vcols_;
  string header_line_;
  string template_line_;
};

struct MsgFlow {
  MsgFlow(const string& src, const string& dst): src_(src), dst_(dst) {/**/}
    
  virtual string draw(const DrawContext& dc, size_t draw_index, const MsgFlowOption& mfo) const = 0;

  const string src_;
  const string dst_;
};

struct SeparatorMsgFlow : public MsgFlow {
  SeparatorMsgFlow() : MsgFlow("", "") {/**/} 
  string draw(const DrawContext& dc, size_t draw_index, const MsgFlowOption& mfo) const {
     return dc.template_line_ + "\n";
  }
};

struct ArrowMsgFlow : public MsgFlow {
  ArrowMsgFlow(const string& src, const string& dst, const string& msg_id, const string& extra_info)
      : MsgFlow(src, dst), msg_id_(msg_id), extra_info_(extra_info) {
  }
    
  string draw_arrow_on_template_line(const string& template_line, int start_pos, int end_pos) const {
      string tmp = template_line;
  
      if (start_pos == end_pos) {
        tmp[start_pos] = '*';
      } else {
        int arrow_length = abs(end_pos - start_pos) - 1;
          
        string arrow;
        if (start_pos > end_pos)
          arrow = "<" + string(arrow_length-1, '-');
        else
          arrow = string(arrow_length-1, '-') + ">";
      
        tmp.replace(min(start_pos, end_pos) + 1, arrow.length(), arrow);
      }
  
      return tmp;
  }

  virtual string draw(const DrawContext& dc, size_t draw_index, const MsgFlowOption& mfo) const {
    string ret = draw_arrow_on_template_line(dc.template_line_, dc.vcol_of(src_), dc.vcol_of(dst_));
    ret += msg_id_ + "   ";
    if (mfo.unknwn_msg_as_extra_info_) {
      ret += "[" + strings::from_num(draw_index)  + "] ";
    }
    
    ret += extra_info_ + "\n";
    return ret;
  }

  string msg_id_;
  string extra_info_;
};


struct MFExtractor {
  static string append_parenthess_if_need(const string& s) {
    string tmp = strings::replace_all(s, "\\(", "");
           tmp = strings::replace_all(tmp, "\\)", "");
           
    size_t lcount = std::count(tmp.begin(), tmp.end(), '(');
    size_t rcount = std::count(tmp.begin(), tmp.end(), ')');

    if ((0 == lcount) || (lcount > k_max_info_extract_group) || (lcount != rcount)) 
        return "";

    tmp = s;
    if (lcount < k_max_info_extract_group) 
      for (int i = 0; i < k_max_info_extract_group - lcount; ++i) 
        tmp += "()";

    return tmp;
  }
    
  static MsgFlow* extract_msg_flow( const string& line
                                  , const MsgFlowOption& mfo
                                  , const ExtractRule& extract_rule) {
    if (mfo.separator_line_.length() != 0  && mfo.separator_line_ == line) {
       return new SeparatorMsgFlow();
    }
    
    string info_extract_regex = append_parenthess_if_need(extract_rule.info_extract_regex_);
    if (info_extract_regex.empty())
      return NULL;

    std::smatch w;
    bool matched = std::regex_match(line, w, std::regex(info_extract_regex));

    if (!matched)
      return NULL;
    
    string reformat_to = extract_rule.reformat_to_;
    string from = "@";
    for (int i = 0; i < k_max_info_extract_group; ++i) {
      char index_char = '1' + i;
      reformat_to = strings::replace_all(reformat_to, from+index_char, w[i+1]);
    }

    std::smatch what;
    if (std::regex_match(reformat_to, what, std::regex("^src:(.*), dst:(.*), msg_id:(.*), extra_info:(.*)$"))) {
      return new ArrowMsgFlow(what[1], what[2], what[3], what[4]);
    }
    
    return NULL;
  }
};

typedef list<MsgFlow*> MsgFlows;

Actors sort_actors(const MsgFlows& mfs, const MsgFlowOption& mfo, string main_actor) {
  Actors actors;
  for (MsgFlows::const_iterator i = mfs.begin(); i != mfs.end(); ++i) {
    
    Actors::iterator actor_iter = std::find(actors.begin(), actors.end(), (*i)->src_);
    if (actor_iter == actors.end() && main_actor != (*i)->src_ && (*i)->src_.length() != 0) {
      actors.push_back((*i)->src_);
    }

    actor_iter = std::find(actors.begin(), actors.end(), (*i)->dst_);
    if (actor_iter == actors.end() && main_actor != (*i)->dst_ && (*i)->dst_.length() != 0) {
      actors.push_back((*i)->dst_);
    }
  }

  actors.push_front(main_actor);
  
  if (mfo.draw_from_right_) {
    actors.reverse();
  }
  return actors;
}

bool handled_as_msg_flow_option(const string& line, MsgFlowOption& mfo) {
  ExtractRule er;

  std::smatch what;
  if (std::regex_match(line, what, std::regex("#!MF:regex:(.*),\\s*#!MF:reformat_to:(.*)"))) {
    er.info_extract_regex_ = what[1];
    er.reformat_to_ = what[2];
    mfo.extract_rules_.push_back(er);
    return true;
  }
  
  if (strings::is_start_with(line, k_mf_main_actor)) {
    mfo.main_actor_ = line.substr(strlen(k_mf_main_actor));
    return true;
  }
  
  if (strings::is_start_with(line, k_mf_draw_from_right)) {
    mfo.draw_from_right_ = true;
    return true;
  }

  if (strings::is_start_with(line, k_mf_unknwn_msg_as_extra_info)) {
    mfo.unknwn_msg_as_extra_info_ = true;
    return true;
  }

  if (strings::is_start_with(line, k_mf_spearator_line)) {
    mfo.separator_line_ = line.substr(strlen(k_mf_spearator_line));
    return true;
  }
  
  return false;
}

bool extract_msg_flow_from(const string& line, const MsgFlowOption& mfo, MsgFlows& mfs) {
  for (vector<ExtractRule>::const_iterator i = mfo.extract_rules_.begin(); i != mfo.extract_rules_.end(); ++i) {
    if (MsgFlow* mf = MFExtractor::extract_msg_flow(line, mfo, *i)) {
      mfs.push_back(mf);
      return true;
    }
  }
  return false;
}

string draw_msgflow(const vector<string>& lines) {
  MsgFlows mfs;
  MsgFlowOption mfo;
  int msg_flow_index = 0;
  string unknown_flow_infos;

  for (vector<string>::const_iterator i = lines.begin(); i != lines.end(); ++i) {
    if (0 == i->length())
      continue;
    
    if (!handled_as_msg_flow_option(*i, mfo)) {
      bool extracted = extract_msg_flow_from(*i, mfo, mfs);
      if (extracted) {
        ++msg_flow_index;
      } else {
        unknown_flow_infos += "[" + strings::from_num(msg_flow_index) + "] " + *i + "\n";
      }
    }
  }
  
  DrawContext dc(sort_actors(mfs, mfo, mfo.main_actor_));
  string ret = dc.header_line_;

  int mf_index = 1;
  for (MsgFlows::iterator i = mfs.begin(); i != mfs.end(); ++i, ++mf_index) {
    ret += (*i)->draw(dc, mf_index, mfo);
  }

  if (mfo.unknwn_msg_as_extra_info_)
    ret += unknown_flow_infos;

  return ret;
}

int main(int argc, char** argv) {
  string line;
  vector<string> lines;
  while (std::getline(std::cin, line)) {
    lines.push_back(line);
  }

  cout << draw_msgflow(lines);
  
  return 0;
}

