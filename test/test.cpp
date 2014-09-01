#include <iostream>
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include "re2/re2.h"
USING_MOCKCPP_NS;
using namespace std;

std::string exec(const string& cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}

string & replace(string & subj, const string& old, const string& news) {
    size_t uiui = subj.find(old);
    if (uiui != string::npos){
       subj.erase(uiui, old.size());
       subj.insert(uiui, news);
    }
    return subj;
}

TEST(xxx, msg_flow__should__draw_from_left_to_right___by_default) {
  string input = "[sys1] ---->   [sys2] 00001  MSG_0  0x00010002 0x00030004\n"
                 "[sys2] ---->   [sys3] 00002  MSG_1  0x00010002 0x00030004\n"
                 "[sys3] ---->   [sys1] 00003  MSG_2  0x00010002 0x00030004\n";
  
  string cmd_template = "echo \"#!MF:regex:.*\\[(\\w+)\\].*---->.*\\[(\\w+)\\] (.+?)  (.+?)  (.*)"
                               ", #!MF:reformat_to:src:@1, dst:@2, msg_id:@4, extra_info:[@5]\n"
                               "#!MF:main_actor:sys1\n"
                               "__content__\" | ./msgflow";

  string ret = exec(replace(cmd_template, "__content__", input));

  EXPECT_STREQ("sys1   sys2   sys3   \n"
               "  |----->|      |     MSG_0   [0x00010002 0x00030004]\n"
               "  |      |----->|     MSG_1   [0x00010002 0x00030004]\n"
               "  |<------------|     MSG_2   [0x00010002 0x00030004]\n"
	      , ret.c_str());
}

TEST(xxx, msg_flow__should_support___draw_from_right___option) {
  string input = "[sys1] ---->   [sys2] 00001  MSG_0  0x00010002 0x00030004\n"
                 "[sys2] ---->   [sys3] 00002  MSG_1  0x00010002 0x00030004\n"
                 "[sys3] ---->   [sys1] 00003  MSG_2  0x00010002 0x00030004\n";
  
  string cmd_template = "echo \"#!MF:regex:.*\\[(\\w+)\\].*---->.*\\[(\\w+)\\] (.+?)  (.+?)  (.*)"
                               ", #!MF:reformat_to:src:@1, dst:@2, msg_id:@4, extra_info:[@5]\n"
                               "#!MF:main_actor:sys1\n"
                               "#!MF:draw_from_right:\n"
                               "__content__\" | ./msgflow";

  string ret = exec(replace(cmd_template, "__content__", input));

  EXPECT_STREQ("sys3   sys2   sys1   \n"
               "  |      |<-----|     MSG_0   [0x00010002 0x00030004]\n"
               "  |<-----|      |     MSG_1   [0x00010002 0x00030004]\n"
               "  |------------>|     MSG_2   [0x00010002 0x00030004]\n"
	      , ret.c_str());
}

TEST(xxx, msg_flow__should__support___multiple__info_extraction_regex) {
  string input = "     * ---->   [sys2] 00001  MSG_0  0x00010002 0x00030004\n"
                 "[sys2] ---->   [sys3] 00002  MSG_1  0x00010002 0x00030004\n"
                 "[sys3] ---->   [sys1] 00003  MSG_2  0x00010002 0x00030004\n";
  
  string cmd_template = "echo \"#!MF:regex:.*\\[(\\w+)\\].*---->.*\\[(\\w+)\\] (.+?)  (.+?)  (.*)"
                               ", #!MF:reformat_to:src:@1, dst:@2, msg_id:@4, extra_info:[@5]\n"
                               "#!MF:regex:.*\\*.*---->.*\\[(\\w+)\\] (.+?)  (.+?)  (.*)"
                               ", #!MF:reformat_to:src:sys1, dst:@1, msg_id:@3, extra_info:[@4]\n"
                               "#!MF:main_actor:sys1\n"
                               "#!MF:draw_from_right:\n"
                               "__content__\" | ./msgflow";

  string ret = exec(replace(cmd_template, "__content__", input));

  EXPECT_STREQ("sys3   sys2   sys1   \n"
               "  |      |<-----|     MSG_0   [0x00010002 0x00030004]\n"
               "  |<-----|      |     MSG_1   [0x00010002 0x00030004]\n"
               "  |------------>|     MSG_2   [0x00010002 0x00030004]\n"
	      , ret.c_str());
}

TEST(xxx, msg_flow__should__support___show_unknown_msgs____as__extra_info) {
  string input = "xxxxxxxxxxxx\n"
                 "[sys1] ---->   [sys2] 00001  MSG_0  0x00010002 0x00030004\n"
                 "yyyyyyyyyyyy\n"
                 "zzzzzzzzzzzz\n"
                 "[sys2] ---->   [sys3] 00002  MSG_1  0x00010002 0x00030004\n"
                 "[sys3] ---->   [sys1] 00003  MSG_2  0x00010002 0x00030004\n"
                 "kkkkkkkkkkkk\n";
  
  string cmd_template = "echo \"#!MF:regex:.*\\[(\\w+)\\].*---->.*\\[(\\w+)\\] (.+?)  (.+?)  (.*)"
                               ", #!MF:reformat_to:src:@1, dst:@2, msg_id:@4, extra_info:[@5]\n"
                               "#!MF:main_actor:sys1\n"
                               "#!MF:unknwn_msg_as_extra_info:\n"
                               "#!MF:draw_from_right:\n"
                               "__content__\" | ./msgflow";

  string ret = exec(replace(cmd_template, "__content__", input));

  EXPECT_STREQ("sys3   sys2   sys1   \n"
               "  |      |<-----|     MSG_0   [1] [0x00010002 0x00030004]\n"
               "  |<-----|      |     MSG_1   [2] [0x00010002 0x00030004]\n"
               "  |------------>|     MSG_2   [3] [0x00010002 0x00030004]\n"
	       "[0] xxxxxxxxxxxx\n"
	       "[1] yyyyyyyyyyyy\n"
	       "[1] zzzzzzzzzzzz\n"
	       "[3] kkkkkkkkkkkk\n"
	      , ret.c_str());
}
