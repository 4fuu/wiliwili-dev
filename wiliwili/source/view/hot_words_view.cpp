//
// Created by lany on 2025/8/17.
//

#include "view/hot_words_view.hpp"
#include "utils/hot_words_detector.hpp"
#include <borealis/core/thread.hpp>
#include <borealis/views/dialog.hpp>
#include <algorithm>
#include <random>

HotWordsView::HotWordsView() {
    this->setAxis(brls::Axis::ROW);
    this->setJustifyContent(brls::JustifyContent::FLEX_START);
    this->setAlignItems(brls::AlignItems::FLEX_START);
    // Note: setWrap is not available in this version of borealis
    this->setMargins(5, 5, 5, 5);
    this->setHeight(100); // 设置固定高度
    
    brls::Logger::debug("HotWordsView: created");
}

HotWordsView::~HotWordsView() {
    stop_auto_update();
    brls::Logger::debug("HotWordsView: destroyed");
}

void HotWordsView::update_hot_words(const std::vector<HotWord>& words) {
    // 清空现有热词
    clear_hot_words();
    
    // 限制显示数量
    size_t display_count = std::min(words.size(), static_cast<size_t>(MAX_DISPLAY_WORDS));
    
    for (size_t i = 0; i < display_count; ++i) {
        auto* label = create_hot_word_label(words[i]);
        if (label) {
            this->addView(label);
            word_labels_.push_back(label);
        }
    }
    
    brls::Logger::debug("HotWordsView: updated with {} words", display_count);
}

void HotWordsView::set_hot_words_detector(std::shared_ptr<HotWordsDetector> detector) {
    hot_words_detector_ = detector;
}

void HotWordsView::start_auto_update() {
    if (is_auto_updating_) {
        return;
    }
    
    is_auto_updating_ = true;
    on_auto_update(); // 立即更新一次
}

void HotWordsView::stop_auto_update() {
    is_auto_updating_ = false;
    if (auto_update_timer_ != 0) {
        brls::cancelDelay(auto_update_timer_);
        auto_update_timer_ = 0;
    }
}

brls::View* HotWordsView::create() {
    return new HotWordsView();
}

brls::Label* HotWordsView::create_hot_word_label(const HotWord& word) {
    auto* label = new brls::Label();
    
    // 设置文本
    label->setText(word.word);
    
    // 根据词频和类型设置样式
    if (word.is_burst) {
        // 爆发热词：红色，较大字体
        label->setTextColor(nvgRGB(255, 100, 100));
        label->setFontSize(18 + std::min(word.count / 2, 8)); // 最大26号字体
    } else {
        // 常规热词：蓝色，中等字体
        label->setTextColor(nvgRGB(100, 150, 255));
        label->setFontSize(14 + std::min(word.count / 3, 6)); // 最大20号字体
    }
    
    // 设置边距
    label->setMargins(3, 2, 3, 2);
    
    // 添加点击事件：显示详细信息
    label->registerClickAction([word](brls::View* view) {
        std::string detail_msg = "热词: " + word.word + "\n";
        detail_msg += "出现次数: " + std::to_string(word.count) + "\n";
        detail_msg += "类型: ";
        detail_msg += (word.is_burst ? "爆发热词" : "常规热词");
        
        auto* dialog = new brls::Dialog(detail_msg);
        dialog->addButton("确定", [](){});
        dialog->open();
        
        return true;
    });
    
    return label;
}

void HotWordsView::clear_hot_words() {
    // 移除所有子视图
    this->clearViews();
    word_labels_.clear();
}

void HotWordsView::on_auto_update() {
    if (!is_auto_updating_ || !hot_words_detector_) {
        return;
    }
    
    // 获取热词数据
    auto words = hot_words_detector_->get_all_hot_words(MAX_DISPLAY_WORDS);
    
    // 在主线程更新UI
    brls::sync([this, words = std::move(words)]() {
        this->update_hot_words(words);
    });
    
    // 设置下次更新定时器
    if (is_auto_updating_) {
        auto_update_timer_ = brls::delay(UPDATE_INTERVAL_MS, [this]() {
            this->on_auto_update();
        });
    }
}