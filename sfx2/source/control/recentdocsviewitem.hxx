/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SFX2_RECENTDOCSVIEWITEM_HXX
#define INCLUDED_SFX2_RECENTDOCSVIEWITEM_HXX

#include <sfx2/thumbnailviewitem.hxx>
#include <tools/link.hxx>
#include <vcl/idle.hxx>

#include <memory>

namespace sfx2
{
    class RecentDocsView;
}

struct RecentDocsEncryptionCheck;

class RecentDocsViewItem final : public ThumbnailViewItem
{
public:
    RecentDocsViewItem(sfx2::RecentDocsView& rView, const OUString& rURL, const OUString& rTitle,
                       std::u16string_view sThumbnailBase64, sal_uInt16 nId,
                       tools::Long nThumbnailSize, bool isReadOnly, bool isPinned);

    virtual ~RecentDocsViewItem() override;

    /// Replaces the placeholder with the real thumbnail, if that hasn't happened yet.
    void loadThumbnailNow();

    /** Updates own highlight status based on the aPoint position.

        Calls the ancestor's updateHighlight, and then takes care of m_bRemoveIconHighlighted.

        Returns rectangle that needs to be invalidated.
    */
    virtual tools::Rectangle updateHighlight(bool bVisible, const Point& rPoint) override;

    /// Text to be used for the tooltip.
    virtual OUString getHelpText() const override;

    virtual void Paint(drawinglayer::processor2d::BaseProcessor2D* pProcessor,
                       const ThumbnailItemAttributes& rAttrs) override;

    virtual void MouseButtonUp(const MouseEvent& rMEvt) override;

    /// Called when the user clicks a document - it will open it.
    void OpenDocument();

    bool isPinned () const { return m_bPinned; }

private:
    sfx2::RecentDocsView& mrParentView;

    /// Return area where is the icon to remove document from the recent documents.
    tools::Rectangle getRemoveIconArea() const;
    tools::Rectangle getPinnedIconArea() const;

    OUString maURL;

    bool m_isReadOnly = false;

    OUString m_sHelpText;

    /// Is the icon that the user can click to remove the document from the recent documents highlighted?
    bool m_bRemoveIconHighlighted;

    Bitmap m_aRemoveRecentBitmap;

    Bitmap m_aRemoveRecentBitmapHighlighted;

    bool m_bPinned;
    bool m_bPinnedIconHighlighted;
    Bitmap m_aPinnedDocumentBitmap;
    Bitmap m_aPinnedDocumentBitmapHighlighted;

    DECL_LINK(LoadThumbnailHdl, Timer*, void);
    DECL_STATIC_LINK(RecentDocsViewItem, EncryptionCheckDoneHdl, void*, void);

    /// Shows the given thumbnail, keeping it centered in the item.
    void setPreview(Bitmap aPreview);
    /// Asks a worker thread to find out if the document is encrypted, as that needs to open it.
    void startEncryptionCheck();

    tools::Long m_nThumbnailSize;
    /// Stored thumbnail, decoded when idle: there can be many, and that would delay first paint.
    OUString m_aThumbnailBase64;
    Idle m_aLoadIdle;
    std::shared_ptr<RecentDocsEncryptionCheck> m_pEncryptionCheck;
};

#endif // INCLUDED_SFX2_RECENTDOCSVIEWITEM_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
