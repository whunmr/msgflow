#include <sstream>
#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include "re2/re2.h"
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
static const char* k_actor_span = "   ";
static const size_t k_max_info_extract_group = 20;

struct MsgFlow {
  string src_;
  string dst_;
  string msg_id_;
  string extra_info_;

  void show() {
    cout << "src: " << src_ << " ,dst_: " << dst_ << " ,msg_id_: " << msg_id_ << " ,extra_info_: " << extra_info_ << endl;
  }
};

struct ExtractRule {
  string info_extract_regex_;  //.*\\[(\\w+)\\].*---->.*\\[(\\w+)\\] (.+?)  (.+?)  (.*)
  string reformat_to_;         //src:s1_, dst:s2_, msg_id:s3_, extra_info:s4_ s5_
};

struct MsgFlowOption {
  MsgFlowOption() : draw_from_right_(false), unknwn_msg_as_extra_info_(false) {/**/}
  
  string main_actor_;
  bool draw_from_right_;
  bool unknwn_msg_as_extra_info_;
  vector<ExtractRule> extract_rules_;
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
    
  static bool extract_msg_flow( const string& line
			    , const ExtractRule& extract_rule
			    , MsgFlow& mf) {
    RE2::Arg argv[k_max_info_extract_group];
    const RE2::Arg* args[k_max_info_extract_group];
    string s[k_max_info_extract_group];
    
    for (int i = 0; i < k_max_info_extract_group; ++i) {
      argv[i] = &s[i];
      args[i] = &argv[i];
    }

    string info_extract_regex = append_parenthess_if_need(extract_rule.info_extract_regex_);
    if (info_extract_regex.empty())
      return false;

    bool matched = RE2::FullMatchN(line, info_extract_regex, args, k_max_info_extract_group);
    if (!matched)
      return false;
    
    string reformat_to = extract_rule.reformat_to_;
    string from = "@";
    for (int i = 0; i < k_max_info_extract_group; ++i) {
      char index_char = '1' + i;
      reformat_to = strings::replace_all(reformat_to, from+index_char, s[i]);
    }

    string split_regex = "^src:(.*), dst:(.*), msg_id:(.*), extra_info:(.*)$";
    return RE2::FullMatch(reformat_to, split_regex, &mf.src_, &mf.dst_, &mf.msg_id_, &mf.extra_info_);
  }
};

typedef list<MsgFlow> MsgFlows;

list<string> sort_actors(const MsgFlows& mfs, const MsgFlowOption& mfo, string main_actor) {
  list<string> actors;
  for (MsgFlows::const_iterator i = mfs.begin(); i != mfs.end(); ++i) {
    list<string>::iterator actor_iter = std::find(actors.begin(), actors.end(), i->src_);
    if (actor_iter == actors.end() && main_actor != i->src_) {
      actors.push_back(i->src_);
    }

    actor_iter = std::find(actors.begin(), actors.end(), i->dst_);
    if (actor_iter == actors.end() && main_actor != i->dst_) {
      actors.push_back(i->dst_);
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
  if (RE2::FullMatch(line, "#!MF:regex:(.*),\\s*#!MF:reformat_to:(.*)", &er.info_extract_regex_, &er.reformat_to_)) {
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
  
  return false;
}

bool extract_msg_flow_from(const string& line, const MsgFlowOption& mfo, MsgFlows& mfs) {
  MsgFlow mf;
  for (vector<ExtractRule>::const_iterator i = mfo.extract_rules_.begin(); i != mfo.extract_rules_.end(); ++i) {
    if (MFExtractor::extract_msg_flow(line, *i, mf)) {
      mfs.push_back(mf);
      return true;
    }
  }
  return false;
}

string draw_arrow_on_template_line(const string& template_line, int start_pos, int end_pos) {
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

int main(int argc, char** argv) {
  MsgFlows mfs;
  MsgFlowOption mfo;
  string line;

  int msg_flow_index = 0;
  string unknown_flow_infos;

  while (std::getline(std::cin, line)) {
    if (0 == line.length())
      continue;
    
    if (!handled_as_msg_flow_option(line, mfo)) {
      bool extracted = extract_msg_flow_from(line, mfo, mfs);
      if (extracted) {
	++msg_flow_index;
      } else {
	unknown_flow_infos += "[" + strings::from_num(msg_flow_index) + "] " + line + "\n";
      }
    }
  }

  list<string> actors = sort_actors(mfs, mfo, mfo.main_actor_);
  map<string, size_t> vcols;
  string ret;
  size_t actor_start_col = 0;
  for (list<string>::iterator i = actors.begin(); i != actors.end(); ++i) {
    ret += *i + k_actor_span;
    vcols[*i] = actor_start_col + (i->length() / 2);
    actor_start_col += i->length() + strlen(k_actor_span);
  }
  ret += "\n";

  string template_line = string(actor_start_col, ' ');
  for (map<string, size_t>::iterator i = vcols.begin(); i != vcols.end(); ++i) {
    template_line[i->second] = '|';
  }

  int mf_index = 1;
  for (MsgFlows::iterator i = mfs.begin(); i != mfs.end(); ++i, ++mf_index) {
    string tmp = draw_arrow_on_template_line(template_line, vcols[i->src_], vcols[i->dst_]);

    ret += tmp + " " + i->msg_id_ + "   ";
    if (mfo.unknwn_msg_as_extra_info_) {
      ret += "[" + strings::from_num(mf_index)  + "] ";
    }
    ret += i->extra_info_ + "\n";
  }

  cout << ret;
  if (mfo.unknwn_msg_as_extra_info_)
    cout << unknown_flow_infos;
  
  return 0;
}

