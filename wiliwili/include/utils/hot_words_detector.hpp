//
// Created by lany on 2025/8/17.
//

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <set>

struct HotWord {
    std::string word;
    int count;
    bool is_burst;
    
    HotWord() : word(""), count(0), is_burst(false) {}
    HotWord(const std::string& w, int c, bool burst = false) 
        : word(w), count(c), is_burst(burst) {}
};

class HotWordsDetector {
public:
    HotWordsDetector();
    ~HotWordsDetector();
    
    // 添加弹幕文本进行分析
    void add_danmaku(const std::string& text);
    
    // 获取常规热词（基于N-gram）
    std::vector<HotWord> get_recent_hot_words(int limit = 10);
    
    // 获取爆发热词（整句话）
    std::vector<HotWord> get_burst_hot_words(int limit = 5);
    
    // 获取所有热词（合并后排序）
    std::vector<HotWord> get_all_hot_words(int limit = 15);
    
    // 清理过期数据
    void cleanup_expired_data();

private:
    struct WordCount {
        int count = 0;
        std::chrono::time_point<std::chrono::steady_clock> last_update;
        
        WordCount() : last_update(std::chrono::steady_clock::now()) {}
    };
    
    struct BurstCount {
        int count = 0;
        std::chrono::time_point<std::chrono::steady_clock> first_seen;
        std::chrono::time_point<std::chrono::steady_clock> last_seen;
        
        BurstCount() {
            auto now = std::chrono::steady_clock::now();
            first_seen = now;
            last_seen = now;
        }
    };
    
    // 常规热词统计（N-gram）
    std::map<std::string, WordCount> recent_words_;
    
    // 爆发热词统计（整句话）
    std::map<std::string, BurstCount> burst_words_;
    
    // 停用词集合
    std::set<std::string> stop_words_;
    
    // 时间窗口配置
    static const int RECENT_TIME_WINDOW = 60;    // 常规热词60秒窗口
    static const int BURST_TIME_WINDOW = 15;     // 爆发热词15秒窗口
    static const int BURST_THRESHOLD = 3;        // 3人以上发同样内容算爆发
    static const int MIN_WORD_LENGTH = 1;        // 最小词长度
    static const int MAX_WORD_LENGTH = 10;       // 最大词长度
    
    // 辅助方法
    std::vector<std::string> generate_ngrams(const std::string& text, int n);
    bool is_valid_word(const std::string& word);
    std::string preprocess_text(const std::string& text);
    void init_stop_words();
    bool is_expired(const std::chrono::time_point<std::chrono::steady_clock>& time_point, int window_seconds);
};