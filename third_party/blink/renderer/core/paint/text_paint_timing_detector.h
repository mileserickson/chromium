// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TEXT_PAINT_TIMING_DETECTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TEXT_PAINT_TIMING_DETECTOR_H_

#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {
class PaintLayer;
class IntRect;
class LayoutObject;
class TracedValue;
class LocalFrameView;

struct TextRecord {
  DOMNodeId node_id;
  double first_size = 0.0;
  base::TimeTicks first_paint_time;
  String text = "";
};

struct TextRect {
  LayoutRect invalidated_rect;
  IntRect transformed_rect_in_viewport;
};

// TextPaintTimingDetector contains Largest Text Paint and Last Text Paint.
//
// Largest Text Paint timing measures when the largest text element gets painted
// within viewport. Last Text Paint timing measures when the last text element
// gets painted within viewport. Specifically, they:
// 1. Tracks all texts' first invalidation, recording their visual size, paint
// time.
// 2. Every 1 second after the first text pre-paint, the algorithm starts an
// analysis. In the analysis:
// 2.1 Largest Text Paint finds the text with the
// largest first visual size, reports its first paint time as a candidate
// result.
// 2.2 Last Text Paint finds the text with the largest first paint time,
// report its first paint time as a candidate result.
//
// For all these candidate results, Telemetry picks the lastly reported
// Largest Text Paint candidate and Last Text Paint candidate respectively as
// their final result.
//
// See also:
// https://docs.google.com/document/d/1DRVd4a2VU8-yyWftgOparZF-sf16daf0vfbsHuz2rws/edit#heading=h.lvno2v283uls
class CORE_EXPORT TextPaintTimingDetector final
    : public GarbageCollectedFinalized<TextPaintTimingDetector> {
  using ReportTimeCallback =
      WTF::CrossThreadFunction<void(WebLayerTreeView::SwapResult,
                                    base::TimeTicks)>;
  friend class TextPaintTimingDetectorTest;

 public:
  TextPaintTimingDetector(LocalFrameView* frame_view);
  void RecordText(const LayoutObject& object, const PaintLayer& painting_layer);
  TextRecord* FindLargestPaintCandidate();
  TextRecord* FindLastPaintCandidate();
  void OnPrePaintFinished();
  void NotifyNodeRemoved(DOMNodeId);
  void Dispose() { timer_.Stop(); }
  void Trace(blink::Visitor*);

 private:
  void PopulateTraceValue(std::unique_ptr<TracedValue>& value,
                          TextRecord* first_text_paint,
                          int report_count);
  IntRect CalculateTransformedRect(LayoutRect& visual_rect,
                                   const PaintLayer& painting_layer);
  void TimerFired(TimerBase*);

  void ReportSwapTime(WebLayerTreeView::SwapResult result,
                      base::TimeTicks timestamp);
  void RegisterNotifySwapTime(ReportTimeCallback callback);

  HashSet<DOMNodeId> recorded_text_node_ids;
  HashSet<DOMNodeId> size_zero_node_ids;
  std::priority_queue<std::unique_ptr<TextRecord>,
                      std::vector<std::unique_ptr<TextRecord>>,
                      std::function<bool(std::unique_ptr<TextRecord>&,
                                         std::unique_ptr<TextRecord>&)>>
      largest_text_heap_;
  std::priority_queue<std::unique_ptr<TextRecord>,
                      std::vector<std::unique_ptr<TextRecord>>,
                      std::function<bool(std::unique_ptr<TextRecord>&,
                                         std::unique_ptr<TextRecord>&)>>
      latest_text_heap_;
  std::vector<TextRecord> texts_to_record_swap_time;

  unsigned largest_text_report_count_ = 0;
  unsigned last_text_report_count_ = 0;
  TaskRunnerTimer<TextPaintTimingDetector> timer_;
  Member<LocalFrameView> frame_view_;
};
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_TEXT_PAINT_TIMING_DETECTOR_H_
