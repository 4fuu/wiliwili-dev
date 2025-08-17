//
// Created by lany on 2025/8/17.
//

#pragma once

#include <borealis/core/box.hpp>
#include <borealis/core/bind.hpp>
#include <borealis/views/label.hpp>
#include <vector>
#include <memory>

class HotWordsDetector;
struct HotWord;

class HotWordsView : public brls::Box {
public:
    HotWordsView();
    ~HotWordsView() override;
    
    // 更新热词显示
    void update_hot_words(const std::vector<HotWord>& words);
    
    // 设置热词检测器（用于获取数据）
    void set_hot_words_detector(std::shared_ptr<HotWordsDetector> detector);
    
    // 启动/停止自动更新
    void start_auto_update();
    void stop_auto_update();
    
    static brls::View* create();

private:
    // 创建热词标签
    brls::Label* create_hot_word_label(const HotWord& word);
    
    // 清空所有热词标签
    void clear_hot_words();
    
    // 自动更新定时器回调
    void on_auto_update();
    
    std::shared_ptr<HotWordsDetector> hot_words_detector_;
    std::vector<brls::Label*> word_labels_;
    
    // 自动更新定时器
    size_t auto_update_timer_ = 0;
    bool is_auto_updating_ = false;
    
    // 配置
    static const int MAX_DISPLAY_WORDS = 12;
    static const int UPDATE_INTERVAL_MS = 3000; // 3秒更新一次
};