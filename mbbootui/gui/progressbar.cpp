// progressbar.cpp - GUIProgressBar object

#include "gui/progressbar.hpp"

#include "mblog/logging.h"

#include "data.hpp"
#include "variables.h"

GUIProgressBar::GUIProgressBar(xml_node<>* node) : GUIObject(node)
{
    xml_node<>* child;

    mEmptyBar = nullptr;
    mFullBar = nullptr;
    mLastPos = 0;
    mSlide = 0.0;
    mSlideInc = 0.0;

    if (!node) {
        LOGE("GUIProgressBar created without XML node");
        return;
    }

    child = FindNode(node, "resource");
    if (child) {
        mEmptyBar = LoadAttrImage(child, "empty");
        mFullBar = LoadAttrImage(child, "full");
    }

    // Load the placement
    LoadPlacement(FindNode(node, "placement"), &mRenderX, &mRenderY);

    // Load the data
    child = FindNode(node, "data");
    if (child) {
        mMinValVar = LoadAttrString(child, "min");
        mMaxValVar = LoadAttrString(child, "max");
        mCurValVar = LoadAttrString(child, "name");
    }

    mRenderW = mEmptyBar->GetWidth();
    mRenderH = mEmptyBar->GetHeight();
}

int GUIProgressBar::Render()
{
    if (!isConditionTrue()) {
        return 0;
    }

    // This handles making sure timing updates occur
    Update();
    return RenderInternal();
}

int GUIProgressBar::RenderInternal()
{
    if (!mEmptyBar || !mEmptyBar->GetResource()) {
        return -1;
    }

    if (!mFullBar || !mFullBar->GetResource()) {
        return -1;
    }

    gr_blit(mEmptyBar->GetResource(), 0, 0, mRenderW, mRenderH, mRenderX, mRenderY);
    gr_blit(mFullBar->GetResource(), 0, 0, mLastPos, mRenderH, mRenderX, mRenderY);
    return 0;
}

int GUIProgressBar::Update()
{
    if (!isConditionTrue()) {
        return 0;
    }

    std::string str;
    int min, max, cur, pos;

    if (mMinValVar.empty()) {
        min = 0;
    } else {
        str.clear();
        if (atoi(mMinValVar.c_str()) != 0) {
            str = mMinValVar;
        } else {
            DataManager::GetValue(mMinValVar, str);
        }
        min = atoi(str.c_str());
    }

    if (mMaxValVar.empty()) {
        max = 100;
    } else {
        str.clear();
        if (atoi(mMaxValVar.c_str()) != 0) {
            str = mMaxValVar;
        } else {
            DataManager::GetValue(mMaxValVar, str);
        }
        max = atoi(str.c_str());
    }

    str.clear();
    DataManager::GetValue(mCurValVar, str);
    cur = atoi(str.c_str());

    // Do slide, if needed
    if (mSlideFrames) {
        mSlide += mSlideInc;
        mSlideFrames--;
        if (cur != (int) mSlide) {
            cur = (int) mSlide;
            DataManager::SetValue(mCurValVar, cur);
        }
    }

    // Normalize to 0
    max -= min;
    cur -= min;
    min = 0;

    if (cur < min) {
        cur = min;
    }
    if (cur > max) {
        cur = max;
    }

    if (max == 0) {
        pos = 0;
    } else {
        pos = (cur * mRenderW) / max;
    }

    if (pos == mLastPos) {
        return 0;
    }

    mLastPos = pos;

    if (RenderInternal() != 0) {
        return -1;
    }
    return 2;
}

int GUIProgressBar::NotifyVarChange(const std::string& varName,
                                    const std::string& value)
{
    GUIObject::NotifyVarChange(varName, value);

    if (!isConditionTrue()) {
        return 0;
    }

    static int nextPush = 0;

    if (varName.empty()) {
        nextPush = 0;
        mLastPos = 0;
        mSlide = 0.0;
        mSlideInc = 0.0;
        return 0;
    }

    if (varName == VAR_TW_UI_PROGRESS_PORTION
            || varName == VAR_TW_UI_PROGRESS_FRAMES) {
        std::string str;
        int cur;

        if (mSlideFrames) {
            mSlide += (mSlideInc * mSlideFrames);
            cur = (int) mSlide;
            DataManager::SetValue(mCurValVar, cur);
            mSlideFrames = 0;
        }

        if (nextPush) {
            mSlide += nextPush;
            cur = (int) mSlide;
            DataManager::SetValue(mCurValVar, cur);
            nextPush = 0;
        }

        if (varName == VAR_TW_UI_PROGRESS_PORTION) {
            mSlide = atof(value.c_str());
        } else {
            mSlideFrames = atol(value.c_str());
            if (mSlideFrames == 0) {
                // We're just holding this progress until the next push
                nextPush = mSlide;
            }
        }

        if (mSlide > 0 && mSlideFrames > 0) {
            // Get the current position
            str.clear();
            DataManager::GetValue(mCurValVar, str);
            cur = atoi(str.c_str());

            mSlideInc = (float) mSlide / (float) mSlideFrames;
            mSlide = cur;
        }
    }
    return 0;
}
