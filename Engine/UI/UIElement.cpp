//
// Urho3D Engine
// Copyright (c) 2008-2011 Lasse ��rni
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Precompiled.h"
#include "Log.h"
#include "ResourceCache.h"
#include "UIElement.h"
#include "UIEvents.h"

#include "DebugNew.h"

std::string UIElement::sClipBoard;

UIElement::UIElement(const std::string& name) :
    mName(name),
    mParent(0),
    mOrigin(0),
    mClipBorder(IntRect::sZero),
    mPriority(0),
    mOpacity(1.0f),
    mBringToFront(false),
    mBringToBack(true),
    mClipChildren(false),
    mEnabled(false),
    mFocusMode(FM_NOTFOCUSABLE),
    mFocus(false),
    mSelected(false),
    mVisible(true),
    mHovering(false),
    mLayoutOrientation(O_VERTICAL),
    mHorizontalLayoutMode(LM_FREE),
    mVerticalLayoutMode(LM_FREE),
    mLayoutSpacing(0),
    mLayoutBorder(IntRect::sZero),
    mResizeNestingLevel(0),
    mUpdateLayoutNestingLevel(0),
    mPosition(IntVector2::sZero),
    mSize(IntVector2::sZero),
    mMinSize(IntVector2::sZero),
    mMaxSize(M_MAX_INT, M_MAX_INT),
    mChildOffset(IntVector2::sZero),
    mHorizontalAlignment(HA_LEFT),
    mVerticalAlignment(VA_TOP),
    mScreenPositionDirty(true),
    mDerivedOpacityDirty(true),
    mHasColorGradient(false)
{
}

UIElement::~UIElement()
{
    // If child elements have outside references, detach them
    while (mChildren.size())
    {
        const SharedPtr<UIElement>& element = mChildren.back();
        if (element.getRefCount() > 1)
        {
            element->mParent = 0;
            element->markDirty();
        }
        mChildren.pop_back();
    }
}

void UIElement::setStyle(const XMLElement& element, ResourceCache* cache)
{
    if (!cache)
        return;
    
    if (element.hasAttribute("name"))
        mName = element.getString("name");
    if (element.hasChildElement("position"))
        setPosition(element.getChildElement("position").getIntVector2("value"));
    if (element.hasChildElement("size"))
        setSize(element.getChildElement("size").getIntVector2("value"));
    if (element.hasChildElement("minsize"))
        setMinSize(element.getChildElement("minsize").getIntVector2("value"));
    if (element.hasChildElement("maxsize"))
        setMaxSize(element.getChildElement("maxsize").getIntVector2("value"));
    if (element.hasChildElement("fixedsize"))
        setFixedSize(element.getChildElement("fixedsize").getIntVector2("value"));
    if (element.hasChildElement("alignment"))
    {
        XMLElement alignElem = element.getChildElement("alignment");
        
        std::string horiz;
        std::string vert;
        if (alignElem.hasAttribute("h"))
            horiz = alignElem.getStringLower("h");
        if (alignElem.hasAttribute("v"))
            vert = alignElem.getStringLower("v");
        if (alignElem.hasAttribute("horizontal"))
            horiz = alignElem.getStringLower("horizontal");
        if (alignElem.hasAttribute("vertical"))
            vert = alignElem.getStringLower("vertical");
        if (horiz == "left")
            setHorizontalAlignment(HA_LEFT);
        if (horiz == "center")
            setHorizontalAlignment(HA_CENTER);
        if (horiz == "right")
            setHorizontalAlignment(HA_RIGHT);
        if (vert == "top")
            setVerticalAlignment(VA_TOP);
        if (vert == "center")
            setVerticalAlignment(VA_CENTER);
        if (vert == "bottom")
            setVerticalAlignment(VA_BOTTOM);
    }
    if (element.hasChildElement("clipborder"))
        setClipBorder(element.getChildElement("clipborder").getIntRect("value"));
    if (element.hasChildElement("priority"))
        setPriority(element.getChildElement("priority").getInt("value"));
    if (element.hasChildElement("opacity"))
        setOpacity(element.getChildElement("opacity").getFloat("value"));
    if (element.hasChildElement("color"))
    {
        XMLElement colorElem = element.getChildElement("color");
        if (colorElem.hasAttribute("value"))
            setColor(colorElem.getColor("value"));
        if (colorElem.hasAttribute("topleft"))
            setColor(C_TOPLEFT, colorElem.getColor("topleft"));
        if (colorElem.hasAttribute("topright"))
            setColor(C_TOPRIGHT, colorElem.getColor("topright"));
        if (colorElem.hasAttribute("bottomleft"))
            setColor(C_BOTTOMLEFT, colorElem.getColor("bottomleft"));
        if (colorElem.hasAttribute("bottomright"))
            setColor(C_BOTTOMRIGHT, colorElem.getColor("bottomright"));
    }
    if (element.hasChildElement("bringtofront"))
        setBringToFront(element.getChildElement("bringtofront").getBool("enable"));
    if (element.hasChildElement("bringtoback"))
        setBringToBack(element.getChildElement("bringtoback").getBool("enable"));
    if (element.hasChildElement("clipchildren"))
        setClipChildren(element.getChildElement("clipchildren").getBool("enable"));
    if (element.hasChildElement("enabled"))
        setEnabled(element.getChildElement("enabled").getBool("enable"));
    if (element.hasChildElement("focusmode"))
    {
        std::string focusMode = element.getChildElement("focusmode").getStringLower("value");
        if (focusMode == "notfocusable")
            setFocusMode(FM_NOTFOCUSABLE);
        if (focusMode == "resetfocus")
            setFocusMode(FM_RESETFOCUS);
        if (focusMode == "focusable")
            setFocusMode(FM_FOCUSABLE);
        if ((focusMode == "focusabledefocusable") || (focusMode == "defocusable"))
            setFocusMode(FM_FOCUSABLE_DEFOCUSABLE);
    }
    if (element.hasChildElement("selected"))
        setSelected(element.getChildElement("selected").getBool("enable"));
    if (element.hasChildElement("visible"))
        setVisible(element.getChildElement("visible").getBool("enable"));
    if (element.hasChildElement("layout"))
    {
        XMLElement layoutElem = element.getChildElement("layout");
        std::string orientation = layoutElem.getStringLower(orientation);
        if ((orientation == "horizontal") || (orientation == "h"))
            mLayoutOrientation = O_HORIZONTAL;
        if ((orientation == "vertical") || (orientation == "v"))
            mLayoutOrientation = O_VERTICAL;
        
        std::string horiz;
        std::string vert;
        if (layoutElem.hasAttribute("h"))
            horiz = layoutElem.getStringLower("h");
        if (layoutElem.hasAttribute("v"))
            vert = layoutElem.getStringLower("v");
        if (layoutElem.hasAttribute("horizontal"))
            horiz = layoutElem.getStringLower("horizontal");
        if (layoutElem.hasAttribute("vertical"))
            vert = layoutElem.getStringLower("vertical");
        if (horiz == "free")
            mHorizontalLayoutMode = LM_FREE;
        if ((horiz == "children") || (horiz == "resizechildren"))
            mHorizontalLayoutMode = LM_RESIZECHILDREN;
        if ((horiz == "element") || (horiz == "resizeelement"))
            mHorizontalLayoutMode = LM_RESIZEELEMENT;
        if (vert == "free")
            mVerticalLayoutMode = LM_FREE;
        if ((vert == "children") || (vert == "resizechildren"))
            mVerticalLayoutMode = LM_RESIZECHILDREN;
        if ((vert == "element") || (vert == "resizeelement"))
            mVerticalLayoutMode = LM_RESIZEELEMENT;
        
        if (layoutElem.hasAttribute("spacing"))
            mLayoutSpacing = max(layoutElem.getInt("spacing"), 0);
        if (layoutElem.hasAttribute("border"))
            mLayoutBorder = layoutElem.getIntRect("border");
        
        updateLayout();
    }
}

void UIElement::update(float timeStep)
{
}

void UIElement::getBatches(std::vector<UIBatch>& batches, std::vector<UIQuad>& quads, const IntRect& currentScissor)
{
    // Reset hovering for next frame
    mHovering = false;
}

IntVector2 UIElement::getScreenPosition()
{
    if (mScreenPositionDirty)
    {
        IntVector2 pos = mPosition;
        const UIElement* parent = mParent;
        const UIElement* current = this;
        
        while (parent)
        {
            switch (current->mHorizontalAlignment)
            {
            case HA_LEFT:
                pos.mX += parent->mPosition.mX;
                break;
                
            case HA_CENTER:
                pos.mX += parent->mPosition.mX + parent->mSize.mX / 2 - current->mSize.mX / 2;
                break;
                
            case HA_RIGHT:
                pos.mX += parent->mPosition.mX + parent->mSize.mX - current->mSize.mX;
                break;
            }
            switch (current->mVerticalAlignment)
            {
            case VA_TOP:
                pos.mY += parent->mPosition.mY;
                break;
                
            case VA_CENTER:
                pos.mY += parent->mPosition.mY + parent->mSize.mY / 2 - current->mSize.mY / 2;
                break;
                
            case VA_BOTTOM:
                pos.mY += parent->mPosition.mY + parent->mSize.mY - current->mSize.mY;
                break;
            }
            
            pos += parent->mChildOffset;
            
            current = parent;
            parent = parent->mParent;
        }
        
        mScreenPosition = pos;
        mScreenPositionDirty = false;
    }
    
    return mScreenPosition;
}

float UIElement::getDerivedOpacity()
{
    if (mDerivedOpacityDirty)
    {
        float opacity = mOpacity;
        const UIElement* parent = mParent;
        
        while (parent)
        {
            opacity *= parent->mOpacity;
            parent = parent->mParent;
        }
        
        mDerivedOpacity = opacity;
        mDerivedOpacityDirty = false;
    }
    
    return mDerivedOpacity;
}

void UIElement::onHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers)
{
    mHovering = true;
}

void UIElement::onClick(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers)
{
}

void UIElement::onDragStart(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers)
{
}

void UIElement::onDragMove(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers)
{
}

void UIElement::onDragEnd(const IntVector2& position, const IntVector2& screenPosition)
{
}

void UIElement::onKey(int key, int buttons, int qualifiers)
{
}

void UIElement::onChar(unsigned char c, int buttons, int qualifiers)
{
}

void UIElement::onResize()
{
}

void UIElement::onFocus()
{
}

void UIElement::onDefocus()
{
}

void UIElement::setName(const std::string& name)
{
    mName = name;
}

void UIElement::setPosition(const IntVector2& position)
{
    if (position != mPosition)
    {
        mPosition = position;
        markDirty();
    }
}

void UIElement::setPosition(int x, int y)
{
    setPosition(IntVector2(x, y));
}

void UIElement::setSize(const IntVector2& size)
{
    ++mResizeNestingLevel;
    
    IntVector2 validatedSize;
    validatedSize.mX = clamp(size.mX, mMinSize.mX, mMaxSize.mX);
    validatedSize.mY = clamp(size.mY, mMinSize.mY, mMaxSize.mY);
    
    if (validatedSize != mSize)
    {
        mSize = validatedSize;
        
        if (mResizeNestingLevel == 1)
        {
            // Check if parent element's layout needs to be updated
            if (mParent)
                mParent->updateLayout();
            
            markDirty();
            onResize();
            
            using namespace Resized;
            
            VariantMap eventData;
            eventData[P_ELEMENT] = (void*)this;
            sendEvent(mFocus ? EVENT_FOCUSED : EVENT_DEFOCUSED, eventData);
            
            updateLayout();
        }
    }
    
    --mResizeNestingLevel;
}

void UIElement::setSize(int width, int height)
{
    setSize(IntVector2(width, height));
}

void UIElement::setWidth(int width)
{
    setSize(IntVector2(width, mSize.mY));
}

void UIElement::setHeight(int height)
{
    setSize(IntVector2(mSize.mX, height));
}


void UIElement::setMinSize(const IntVector2& minSize)
{
    mMinSize.mX = max(minSize.mX, 0);
    mMinSize.mY = max(minSize.mY, 0);
    mMaxSize.mX = max(minSize.mX, mMaxSize.mX);
    mMaxSize.mY = max(minSize.mY, mMaxSize.mY);
    setSize(mSize);
}

void UIElement::setMinSize(int width, int height)
{
    setMinSize(IntVector2(width, height));
}

void UIElement::setMinWidth(int width)
{
    setMaxSize(IntVector2(width, mMinSize.mY));
}

void UIElement::setMinHeight(int height)
{
    setMaxSize(IntVector2(mMinSize.mX, height));
}

void UIElement::setMaxSize(const IntVector2& maxSize)
{
    mMaxSize.mX = max(mMinSize.mX, maxSize.mX);
    mMaxSize.mY = max(mMinSize.mY, maxSize.mY);
    setSize(mSize);
}

void UIElement::setMaxSize(int width, int height)
{
    setMaxSize(IntVector2(width, height));
}

void UIElement::setMaxWidth(int width)
{
    setMaxSize(IntVector2(width, mMaxSize.mY));
}

void UIElement::setMaxHeight(int height)
{
    setMaxSize(IntVector2(mMaxSize.mX, height));
}

void UIElement::setFixedSize(const IntVector2& size)
{
    mMinSize = mMaxSize = IntVector2(max(size.mX, 0), max(size.mY, 0));
    setSize(size);
}

void UIElement::setFixedSize(int width, int height)
{
    setFixedSize(IntVector2(width, height));
}

void UIElement::setFixedWidth(int width)
{
    mMinSize.mX = mMaxSize.mX = max(width, 0);
    setWidth(width);
}

void UIElement::setFixedHeight(int height)
{
    mMinSize.mY = mMaxSize.mY = max(height, 0);
    setHeight(height);
}

void UIElement::setAlignment(HorizontalAlignment hAlign, VerticalAlignment vAlign)
{
    mHorizontalAlignment = hAlign;
    mVerticalAlignment = vAlign;
    markDirty();
}

void UIElement::setHorizontalAlignment(HorizontalAlignment align)
{
    mHorizontalAlignment = align;
    markDirty();
}

void UIElement::setVerticalAlignment(VerticalAlignment align)
{
    mVerticalAlignment = align;
    markDirty();
}

void UIElement::setClipBorder(const IntRect& rect)
{
    mClipBorder.mLeft = max(rect.mLeft, 0);
    mClipBorder.mTop = max(rect.mTop, 0);
    mClipBorder.mRight = max(rect.mRight, 0);
    mClipBorder.mBottom = max(rect.mBottom, 0);
}

void UIElement::setClipBorder(int left, int top, int right, int bottom)
{
    setClipBorder(IntRect(left, top, right, bottom));
}

void UIElement::setColor(const Color& color)
{
    for (unsigned i = 0; i < MAX_UIELEMENT_CORNERS; ++i)
        mColor[i] = color;
    mHasColorGradient = false;
}

void UIElement::setColor(Corner corner, const Color& color)
{
    mColor[corner] = color;
    mHasColorGradient = false;
    
    for (unsigned i = 0; i < MAX_UIELEMENT_CORNERS; ++i)
    {
        if ((i != corner) && (mColor[i] != mColor[corner]))
            mHasColorGradient = true;
    }
}

void UIElement::setPriority(int priority)
{
    mPriority = priority;
}

void UIElement::setOpacity(float opacity)
{
    mOpacity = clamp(opacity, 0.0f, 1.0f);
    markDirty();
}

void UIElement::setBringToFront(bool enable)
{
    mBringToFront = enable;
}

void UIElement::setBringToBack(bool enable)
{
    mBringToBack = enable;
}

void UIElement::setClipChildren(bool enable)
{
    mClipChildren = enable;
}

void UIElement::setEnabled(bool enable)
{
    mEnabled = enable;
}

void UIElement::setFocusMode(FocusMode mode)
{
    mFocusMode = mode;
}

void UIElement::setFocus(bool enable)
{
    if (mFocusMode < FM_FOCUSABLE)
        enable = false;
    
    if (enable != mFocus)
    {
        mFocus = enable;
        
        if (enable)
            onFocus();
        else
            onDefocus();
        
        using namespace Focused;
        
        VariantMap eventData;
        eventData[P_ELEMENT] = (void*)this;
        sendEvent(mFocus ? EVENT_FOCUSED : EVENT_DEFOCUSED, eventData);
    }
}

void UIElement::setSelected(bool enable)
{
    mSelected = enable;
}

void UIElement::setVisible(bool enable)
{
    if (enable != mVisible)
    {
        mVisible = enable;
        
        // Parent's layout may change as a result of visibility change
        if (mParent)
            mParent->updateLayout();
    }
}

void UIElement::setUserData(const Variant& userData)
{
    mUserData = userData;
}

void UIElement::setStyleAuto(XMLFile* file, ResourceCache* cache)
{
    XMLElement element = getStyleElement(file);
    setStyle(element, cache);
}

void UIElement::setLayout(Orientation orientation, LayoutMode horizontal, LayoutMode vertical, int spacing, const IntRect& border)
{
    mLayoutOrientation = orientation;
    mHorizontalLayoutMode = horizontal;
    mVerticalLayoutMode = vertical;
    mLayoutSpacing = max(spacing, 0);
    mLayoutBorder = border;
    
    updateLayout();
}

void UIElement::updateLayout()
{
    if ((mUpdateLayoutNestingLevel) || ((mHorizontalLayoutMode == LM_FREE) && (mVerticalLayoutMode == LM_FREE)))
        return;
    
    ++mUpdateLayoutNestingLevel;
    
    std::vector<int> positions;
    std::vector<int> sizes;
    std::vector<int> minSizes;
    std::vector<int> maxSizes;
    
    if (mLayoutOrientation == O_HORIZONTAL)
    {
        int maxChildHeight = 0;
        
        if (mHorizontalLayoutMode == LM_RESIZEELEMENT)
        {
            for (unsigned i = 0; i < mChildren.size(); ++i)
            {
                if (!mChildren[i]->isVisible())
                    continue;
                sizes.push_back(mChildren[i]->getWidth());
                maxChildHeight = max(maxChildHeight, mChildren[i]->getHeight());
            }
            
            setSize(calculateLayoutParentSize(sizes, mLayoutBorder.mLeft, mLayoutBorder.mRight, mLayoutSpacing), mVerticalLayoutMode ==
                LM_RESIZEELEMENT ? maxChildHeight + mLayoutBorder.mTop + mLayoutBorder.mBottom : getHeight());
           
            int position = mLayoutBorder.mLeft;
            for (unsigned i = 0; i < mChildren.size(); ++i)
            {
                if (!mChildren[i]->isVisible())
                    continue;
                mChildren[i]->setHorizontalAlignment(HA_LEFT);
                mChildren[i]->setPosition(position, getLayoutChildPosition(mChildren[i]).mY);
                if (mVerticalLayoutMode == LM_RESIZECHILDREN)
                    mChildren[i]->setHeight(getHeight() - mLayoutBorder.mTop - mLayoutBorder.mBottom);
                position += mChildren[i]->getWidth();
                position += mLayoutSpacing;
            }
        }
        
        if (mHorizontalLayoutMode == LM_RESIZECHILDREN)
        {
            for (unsigned i = 0; i < mChildren.size(); ++i)
            {
                if (!mChildren[i]->isVisible())
                    continue;
                positions.push_back(0);
                sizes.push_back(mChildren[i]->getWidth());
                minSizes.push_back(mChildren[i]->getMinWidth());
                maxSizes.push_back(mChildren[i]->getMaxWidth());
                maxChildHeight = max(maxChildHeight, mChildren[i]->getHeight());
            }
            
            if (mVerticalLayoutMode == LM_RESIZEELEMENT)
                setHeight(maxChildHeight + mLayoutBorder.mTop + mLayoutBorder.mBottom);
            
            calculateLayout(positions, sizes, minSizes, maxSizes, getWidth(), mLayoutBorder.mLeft, mLayoutBorder.mRight,
                mLayoutSpacing);
            
            unsigned idx = 0;
            for (unsigned i = 0; i < mChildren.size(); ++i)
            {
                if (!mChildren[i]->isVisible())
                    continue;
                mChildren[i]->setHorizontalAlignment(HA_LEFT);
                mChildren[i]->setPosition(positions[idx], getLayoutChildPosition(mChildren[i]).mY);
                mChildren[i]->setSize(sizes[idx], mVerticalLayoutMode == LM_RESIZECHILDREN ? getHeight() - mLayoutBorder.mTop -
                    mLayoutBorder.mBottom : mChildren[i]->getHeight());
                ++idx;
            }
        }
    }
    
    if (mLayoutOrientation == O_VERTICAL)
    {
        int maxChildWidth = 0;
        
        if (mVerticalLayoutMode == LM_RESIZEELEMENT)
        {
            for (unsigned i = 0; i < mChildren.size(); ++i)
            {
                if (!mChildren[i]->isVisible())
                    continue;
                sizes.push_back(mChildren[i]->getHeight());
                maxChildWidth = max(maxChildWidth, mChildren[i]->getWidth());
            }
            
            setSize(mHorizontalLayoutMode == LM_RESIZEELEMENT ? maxChildWidth + mLayoutBorder.mLeft + mLayoutBorder.mRight : getWidth(),
                calculateLayoutParentSize(sizes, mLayoutBorder.mTop, mLayoutBorder.mBottom, mLayoutSpacing));
            
            int position = mLayoutBorder.mTop;
            for (unsigned i = 0; i < mChildren.size(); ++i)
            {
                if (!mChildren[i]->isVisible())
                    continue;
                mChildren[i]->setVerticalAlignment(VA_TOP);
                mChildren[i]->setPosition(getLayoutChildPosition(mChildren[i]).mX, position);
                if (mHorizontalLayoutMode == LM_RESIZECHILDREN)
                    mChildren[i]->setWidth(getWidth() - mLayoutBorder.mLeft - mLayoutBorder.mRight);
                position += mChildren[i]->getHeight();
                position += mLayoutSpacing;
            }
        }
        else
        {
            for (unsigned i = 0; i < mChildren.size(); ++i)
            {
                if (!mChildren[i]->isVisible())
                    continue;
                positions.push_back(0);
                sizes.push_back(mChildren[i]->getHeight());
                minSizes.push_back(mChildren[i]->getMinHeight());
                maxSizes.push_back(mChildren[i]->getMaxHeight());
                maxChildWidth = max(maxChildWidth, mChildren[i]->getWidth());
            }
            
            if (mHorizontalLayoutMode == LM_RESIZEELEMENT)
                setWidth(maxChildWidth + mLayoutBorder.mLeft + mLayoutBorder.mRight);
            
            calculateLayout(positions, sizes, minSizes, maxSizes, getHeight(), mLayoutBorder.mTop, mLayoutBorder.mBottom,
                mLayoutSpacing);
            
            unsigned idx = 0;
            for (unsigned i = 0; i < mChildren.size(); ++i)
            {
                if (!mChildren[i]->isVisible())
                    continue;
                mChildren[i]->setVerticalAlignment(VA_TOP);
                mChildren[i]->setPosition(getLayoutChildPosition(mChildren[i]).mX, positions[idx]);
                mChildren[i]->setSize(mHorizontalLayoutMode == LM_RESIZECHILDREN ? getWidth() - mLayoutBorder.mLeft -
                    mLayoutBorder.mRight : mChildren[i]->getWidth(), sizes[idx]);
                ++idx;
            }
        }
    }
    
    --mUpdateLayoutNestingLevel;
}

void UIElement::bringToFront()
{
    // Follow the parent chain to the top level window. If it has BringToFront mode, bring it to front now
    UIElement* root = getRootElement();
    UIElement* ptr = this;
    while ((ptr) && (ptr->getParent() != root))
        ptr = ptr->getParent();
    if ((!ptr) || (!ptr->getBringToFront()))
        return;
    
    // Get the highest priority used by all other top level elements, decrease their priority by one,
    // and assign that to new front element. However, take into account only active (enabled) elements
    // and those which have the BringToBack flag set
    int maxPriority = M_MIN_INT;
    std::vector<UIElement*> topLevelElements = root->getChildren();
    for (std::vector<UIElement*>::iterator i = topLevelElements.begin(); i != topLevelElements.end(); ++i)
    {
        UIElement* other = *i;
        if ((other->isEnabled()) && (other->getBringToBack()) && (other != ptr))
        {
            int priority = other->getPriority();
            maxPriority = max(priority, maxPriority);
            other->setPriority(priority - 1);
        }
    }
    ptr->setPriority(maxPriority);
}

void UIElement::addChild(UIElement* element)
{
    if ((!element) || (element->mParent == this) || (mParent == element))
        return;
    
    if (element->mParent)
        element->mParent->removeChild(element);
    
    mChildren.push_back(SharedPtr<UIElement>(element));
    
    element->mParent = this;
    element->markDirty();
    updateLayout();
}

void UIElement::removeChild(UIElement* element)
{
    for (std::vector<SharedPtr<UIElement> >::iterator i = mChildren.begin(); i != mChildren.end(); ++i)
    {
        if ((*i) == element)
        {
            element->mParent = 0;
            element->markDirty();
            mChildren.erase(i);
            updateLayout();
            return;
        }
    }
}

void UIElement::removeAllChildren()
{
    while (mChildren.size())
    {
        const SharedPtr<UIElement>& element = mChildren.back();
        element->mParent = 0;
        element->markDirty();
        mChildren.pop_back();
    }
}

std::vector<UIElement*> UIElement::getChildren(bool recursive) const
{
    if (!recursive)
    {
        std::vector<UIElement*> ret;
        for (std::vector<SharedPtr<UIElement> >::const_iterator i = mChildren.begin(); i != mChildren.end(); ++i)
            ret.push_back(*i);
        
        return ret;
    }
    else
    {
        std::vector<UIElement*> allChildren;
        getChildrenRecursive(allChildren);
        
        return allChildren;
    }
}

unsigned UIElement::getNumChildren(bool recursive) const
{
    if (!recursive)
        return mChildren.size();
    else
    {
        unsigned allChildren = mChildren.size();
        for (std::vector<SharedPtr<UIElement> >::const_iterator i = mChildren.begin(); i != mChildren.end(); ++i)
            allChildren += (*i)->getNumChildren(true);
        
        return allChildren;
    }
}

UIElement* UIElement::getChild(unsigned index) const
{
    if (index >= mChildren.size())
        return 0;
    
    return mChildren[index];
}

UIElement* UIElement::getChild(const std::string& name, bool recursive) const
{
    for (std::vector<SharedPtr<UIElement> >::const_iterator i = mChildren.begin(); i != mChildren.end(); ++i)
    {
        if ((*i)->mName == name)
            return *i;
        
        if (recursive)
        {
            UIElement* element = (*i)->getChild(name, true);
            if (element)
                return element;
        }
    }
    
    return 0;
}

UIElement* UIElement::getRootElement() const
{
    UIElement* root = mParent;
    if (!root)
        return 0;
    while (root->getParent())
        root = root->getParent();
    return root;
}

XMLElement UIElement::getStyleElement(XMLFile* file) const
{
    if (file)
    {
        XMLElement rootElem = file->getRootElement();
        XMLElement childElem = rootElem.getChildElement("element");
        while (childElem)
        {
            if (childElem.getString("type") == getTypeName())
                return childElem;
            childElem = childElem.getNextElement("element");
        }
    }
    
    return XMLElement();
}

IntVector2 UIElement::screenToElement(const IntVector2& screenPosition)
{
    return screenPosition - getScreenPosition();
}

IntVector2 UIElement::elementToScreen(const IntVector2& position)
{
    return position + getScreenPosition();
}

bool UIElement::isInside(IntVector2 position, bool isScreen)
{
    if (isScreen)
        position = screenToElement(position);
    return (position.mX >= 0) && (position.mY >= 0) && (position.mX < mSize.mX) && (position.mY < mSize.mY);
}

bool UIElement::isInsideCombined(IntVector2 position, bool isScreen)
{
    // If child elements are clipped, no need to expand the rect
    if (mClipChildren)
        return isInside(position, isScreen);
    
    if (!isScreen)
        position = elementToScreen(position);
    
    IntRect combined = getCombinedScreenRect();
    return (position.mX >= combined.mLeft) && (position.mY >= combined.mTop) && (position.mX < combined.mRight) &&
        (position.mY < combined.mBottom);
}

IntRect UIElement::getCombinedScreenRect()
{
    IntVector2 screenPosition(getScreenPosition());
    IntRect combined(screenPosition.mX, screenPosition.mY, screenPosition.mX + mSize.mX, screenPosition.mY + mSize.mY);
    
    for (std::vector<SharedPtr<UIElement> >::iterator i = mChildren.begin(); i != mChildren.end(); ++i)
    {
        IntVector2 childPos = (*i)->getScreenPosition();
        const IntVector2& childSize = (*i)->getSize();
        if (childPos.mX < combined.mLeft)
            combined.mLeft = childPos.mX;
        if (childPos.mY < combined.mTop)
            combined.mTop = childPos.mY;
        if (childPos.mX + childSize.mX > combined.mRight)
            combined.mRight = childPos.mX + childSize.mX;
        if (childPos.mY + childSize.mY > combined.mBottom)
            combined.mBottom = childPos.mY + childSize.mY;
    }
    
    return combined;
}

void UIElement::setHovering(bool enable)
{
    mHovering = enable;
}

void UIElement::setOrigin(UIElement* origin)
{
    mOrigin = origin;
}

void UIElement::adjustScissor(IntRect& currentScissor)
{
    if (mClipChildren)
    {
        IntVector2 screenPos = getScreenPosition();
        currentScissor.mLeft = max(currentScissor.mLeft, screenPos.mX + mClipBorder.mLeft);
        currentScissor.mTop = max(currentScissor.mTop, screenPos.mY + mClipBorder.mTop);
        currentScissor.mRight = min(currentScissor.mRight, screenPos.mX + mSize.mX - mClipBorder.mRight);
        currentScissor.mBottom = min(currentScissor.mBottom, screenPos.mY + mSize.mY - mClipBorder.mBottom);
        
        if (currentScissor.mRight < currentScissor.mLeft)
            currentScissor.mRight = currentScissor.mLeft;
        if (currentScissor.mBottom < currentScissor.mTop)
            currentScissor.mBottom = currentScissor.mTop;
    }
}

XMLElement UIElement::getStyleElement(XMLFile* file, const std::string& typeName)
{
    if (file)
    {
        XMLElement rootElem = file->getRootElement();
        XMLElement childElem = rootElem.getChildElement("element");
        while (childElem)
        {
            if (childElem.getString("type") == typeName)
                return childElem;
            childElem = childElem.getNextElement("element");
        }
    }
    
    return XMLElement();
}

void UIElement::markDirty()
{
    mScreenPositionDirty = true;
    mDerivedOpacityDirty = true;
    
    for (std::vector<SharedPtr<UIElement> >::const_iterator i = mChildren.begin(); i != mChildren.end(); ++i)
        (*i)->markDirty();
}

void UIElement::setChildOffset(const IntVector2& offset)
{
    if (offset != mChildOffset)
    {
        mChildOffset = offset;
        for (std::vector<SharedPtr<UIElement> >::const_iterator i = mChildren.begin(); i != mChildren.end(); ++i)
            (*i)->markDirty();
    }
}

void UIElement::getChildrenRecursive(std::vector<UIElement*>& dest) const
{
    for (std::vector<SharedPtr<UIElement> >::const_iterator i = mChildren.begin(); i != mChildren.end(); ++i)
    {
        dest.push_back(*i);
        (*i)->getChildrenRecursive(dest);
    }
}

int UIElement::calculateLayoutParentSize(const std::vector<int>& sizes, int begin, int end, int spacing)
{
    int width = begin + end;
    for (unsigned i = 0; i < sizes.size(); ++i)
    {
        width += sizes[i];
        if (i < sizes.size() - 1)
            width += spacing;
    }
    return width;
}

void UIElement::calculateLayout(std::vector<int>& positions, std::vector<int>& sizes, const std::vector<int>& minSizes,
        const std::vector<int>& maxSizes, int targetWidth, int begin, int end, int spacing)
{
    unsigned numChildren = sizes.size();
    if (!numChildren)
        return;
    int targetTotalSize = targetWidth - begin - end - (numChildren - 1) * spacing;
    if (targetTotalSize < 0)
        targetTotalSize = 0;
    int targetChildSize = targetTotalSize / numChildren;
    int remainder = targetTotalSize % numChildren;
    float add = (float)remainder / numChildren;
    float acc = 0.0f;
    
    // Initial pass
    for (unsigned i = 0; i < numChildren; ++i)
    {
        int targetSize = targetChildSize;
        if (remainder)
        {
            acc += add;
            if (acc >= 0.5f)
            {
                acc -= 1.0f;
                ++targetSize;
                --remainder;
            }
        }
        sizes[i] = clamp(targetSize, minSizes[i], maxSizes[i]);
    }
    
    // Error correction passes
    for (;;)
    {
        int actualTotalSize = 0;
        for (unsigned i = 0; i < numChildren; ++i)
            actualTotalSize += sizes[i];
        int error = targetTotalSize - actualTotalSize;
        // Break if no error
        if (!error)
            break;
        
        // Check which of the children can be resized to correct the error. If none, must break
        static std::vector<unsigned> resizable;
        resizable.clear();
        for (unsigned i = 0; i < numChildren; ++i)
        {
            if ((error < 0) && (sizes[i] > minSizes[i]))
                resizable.push_back(i);
            else if ((error > 0) && (sizes[i] < maxSizes[i]))
                resizable.push_back(i);
        }
        if (resizable.empty())
            break;
        
        int errorPerChild = error / resizable.size();
        remainder = (abs(error)) % resizable.size();
        add = (float)remainder / resizable.size();
        acc = 0.0f;
        
        for (unsigned i = 0; i < resizable.size(); ++i)
        {
            unsigned idx = resizable[i];
            int targetSize = sizes[idx] + errorPerChild;
            if (remainder)
            {
                acc += add;
                if (acc >= 0.5f)
                {
                    acc -= 1.0f;
                    targetSize = error < 0 ? targetSize - 1 : targetSize + 1;
                    --remainder;
                }
            }
            
            sizes[idx] = clamp(targetSize, minSizes[idx], maxSizes[idx]);
        }
    }
    
    // Calculate final positions
    int position = begin;
    for (unsigned i = 0; i < numChildren; ++i)
    {
        positions[i] = position;
        position += sizes[i];
        position += spacing;
    }
}

IntVector2 UIElement::getLayoutChildPosition(UIElement* child)
{
    IntVector2 ret(IntVector2::sZero);
    
    HorizontalAlignment ha = child->getHorizontalAlignment();
    switch (ha)
    {
    case HA_LEFT:
        ret.mX = mLayoutBorder.mLeft;
        
    case HA_RIGHT:
        ret.mX -mLayoutBorder.mRight;
    }
    
    VerticalAlignment va = child->getVerticalAlignment();
    switch (va)
    {
    case VA_TOP:
        ret.mY = mLayoutBorder.mTop;
        
    case VA_BOTTOM:
        ret.mY = mLayoutBorder.mBottom;
    }
    return ret;
}

