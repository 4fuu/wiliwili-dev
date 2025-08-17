//
// Created by lany on 2025/8/17.
//

#include "utils/hot_words_detector.hpp"
#include <algorithm>
#include <cctype>

HotWordsDetector::HotWordsDetector() {
    init_stop_words();
}

HotWordsDetector::~HotWordsDetector() = default;

void HotWordsDetector::add_danmaku(const std::string& text) {
    if (text.empty()) return;
    
    auto now = std::chrono::steady_clock::now();
    std::string processed_text = preprocess_text(text);
    
    // 1. N-gram 热词统计
    for (int n = 1; n <= 3; n++) {
        auto ngrams = generate_ngrams(processed_text, n);
        for (const auto& gram : ngrams) {
            if (is_valid_word(gram)) {
                recent_words_[gram].count++;
                recent_words_[gram].last_update = now;
            }
        }
    }
    
    // 2. 爆发热词统计（存储原始整句话）
    if (!text.empty() && text.length() <= 50) { // 限制长度避免刷屏
        auto& burst = burst_words_[text];
        if (burst.count == 0) {
            burst.first_seen = now;
        }
        burst.count++;
        burst.last_seen = now;
    }
    
    // 3. 定期清理过期数据（每100次调用清理一次）
    static int cleanup_counter = 0;
    if (++cleanup_counter >= 100) {
        cleanup_counter = 0;
        cleanup_expired_data();
    }
}

std::vector<HotWord> HotWordsDetector::get_recent_hot_words(int limit) {
    std::vector<HotWord> result;
    
    for (const auto& [word, count_info] : recent_words_) {
        if (!is_expired(count_info.last_update, RECENT_TIME_WINDOW)) {
            result.emplace_back(word, count_info.count, false);
        }
    }
    
    // 按词频排序
    std::sort(result.begin(), result.end(), 
        [](const HotWord& a, const HotWord& b) {
            return a.count > b.count;
        });
    
    if (result.size() > static_cast<size_t>(limit)) {
        result.resize(limit);
    }
    
    return result;
}

std::vector<HotWord> HotWordsDetector::get_burst_hot_words(int limit) {
    std::vector<HotWord> result;
    
    for (const auto& [text, burst_info] : burst_words_) {
        // 检查是否在爆发时间窗口内
        auto duration = std::chrono::duration_cast<std::chrono::seconds>
            (burst_info.last_seen - burst_info.first_seen).count();
        
        // 检查是否满足爆发条件：短时间内多人发同样内容，且最后更新时间不太久
        bool is_recent = !is_expired(burst_info.last_seen, BURST_TIME_WINDOW * 2);
        bool is_burst = (duration <= BURST_TIME_WINDOW && burst_info.count >= BURST_THRESHOLD) ||
                       (burst_info.count >= BURST_THRESHOLD * 2); // 或者数量很多也算爆发
        
        if (is_recent && is_burst) {
            result.emplace_back(text, burst_info.count, true);
        }
    }
    
    // 按词频排序
    std::sort(result.begin(), result.end(), 
        [](const HotWord& a, const HotWord& b) {
            return a.count > b.count;
        });
    
    if (result.size() > static_cast<size_t>(limit)) {
        result.resize(limit);
    }
    
    return result;
}

std::vector<HotWord> HotWordsDetector::get_all_hot_words(int limit) {
    auto recent = get_recent_hot_words(limit);
    auto burst = get_burst_hot_words(limit);
    
    std::vector<HotWord> result;
    result.reserve(recent.size() + burst.size());
    
    // 合并两个列表
    result.insert(result.end(), burst.begin(), burst.end()); // 爆发热词优先
    result.insert(result.end(), recent.begin(), recent.end());
    
    // 去重（避免同一个词既在常规热词又在爆发热词中）
    std::sort(result.begin(), result.end(), 
        [](const HotWord& a, const HotWord& b) {
            if (a.word == b.word) return a.is_burst > b.is_burst; // 优先保留爆发热词
            return a.count > b.count;
        });
    
    auto it = std::unique(result.begin(), result.end(), 
        [](const HotWord& a, const HotWord& b) {
            return a.word == b.word;
        });
    result.erase(it, result.end());
    
    // 重新按词频排序
    std::sort(result.begin(), result.end(), 
        [](const HotWord& a, const HotWord& b) {
            if (a.is_burst != b.is_burst) return a.is_burst > b.is_burst; // 爆发热词优先
            return a.count > b.count;
        });
    
    if (result.size() > static_cast<size_t>(limit)) {
        result.resize(limit);
    }
    
    return result;
}

void HotWordsDetector::cleanup_expired_data() {
    
    // 清理过期的常规热词
    auto recent_it = recent_words_.begin();
    while (recent_it != recent_words_.end()) {
        if (is_expired(recent_it->second.last_update, RECENT_TIME_WINDOW)) {
            recent_it = recent_words_.erase(recent_it);
        } else {
            ++recent_it;
        }
    }
    
    // 清理过期的爆发热词
    auto burst_it = burst_words_.begin();
    while (burst_it != burst_words_.end()) {
        if (is_expired(burst_it->second.last_seen, BURST_TIME_WINDOW * 3)) { // 爆发热词保留更久一点
            burst_it = burst_words_.erase(burst_it);
        } else {
            ++burst_it;
        }
    }
}

std::vector<std::string> HotWordsDetector::generate_ngrams(const std::string& text, int n) {
    std::vector<std::string> ngrams;
    if (text.length() < static_cast<size_t>(n)) {
        return ngrams;
    }
    
    for (size_t i = 0; i <= text.length() - n; ++i) {
        std::string gram = text.substr(i, n);
        if (!gram.empty()) {
            ngrams.push_back(gram);
        }
    }
    
    return ngrams;
}

bool HotWordsDetector::is_valid_word(const std::string& word) {
    if (word.empty() || word.length() < MIN_WORD_LENGTH || word.length() > MAX_WORD_LENGTH) {
        return false;
    }
    
    // 过滤停用词
    if (stop_words_.find(word) != stop_words_.end()) {
        return false;
    }
    
    // 过滤纯空白字符
    if (std::all_of(word.begin(), word.end(), [](unsigned char c) { return std::isspace(c); })) {
        return false;
    }
    
    // 过滤纯数字（但允许特殊数字组合如"666"）
    if (std::all_of(word.begin(), word.end(), [](unsigned char c) { return std::isdigit(c); })) {
        // 允许一些特殊的数字组合
        if (word == "6" || word == "66" || word == "666" || word == "6666" ||
            word == "88" || word == "888" || word == "233" || word == "2333") {
            return true;
        }
        return false;
    }
    
    return true;
}

std::string HotWordsDetector::preprocess_text(const std::string& text) {
    // 简单预处理：去除前后空白
    std::string result = text;
    
    // 去除前后空白
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), result.end());
    
    return result;
}

void HotWordsDetector::init_stop_words() {
    // 添加常见停用词
    stop_words_.insert({
        "的", "了", "在", "是", "我", "你", "他", "她", "它", "们",
        "这", "那", "有", "个", "也", "就", "不", "都", "要", "可",
        "还", "会", "去", "来", "到", "把", "对", "和", "与", "及",
        "为", "被", "从", "向", "往", "给", "让", "使", "叫", "由",
        "但", "而", "或", "则", "却", "又", "再", "才", "即", "若",
        "如", "比", "than", "the", "a", "an", "and", "or", "but",
        " ", "\t", "\n", "\r"
    });
}

bool HotWordsDetector::is_expired(const std::chrono::time_point<std::chrono::steady_clock>& time_point, int window_seconds) {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - time_point).count();
    return duration > window_seconds;
}