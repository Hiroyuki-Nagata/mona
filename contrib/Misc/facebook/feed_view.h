/*
 *
 *   Copyright (c) 2011  Higepon(Taro Minowa)  <higepon@users.sourceforge.jp>
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 *   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 *   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 *   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef CONTRIB_MISC_FACEBOOK_FEED_VIEW_H_
#define CONTRIB_MISC_FACEBOOK_FEED_VIEW_H_

#include <monagui.h>
#include <vector>
#include <string>
#include "./feed.h"

namespace facebook {

typedef std::vector<Component*> Components;

class FeedView {
 public:
  FeedView(int x, int y, int w, int h);
  virtual ~FeedView();

  void components(Components* ret);
  void set_image_path(const std::string& uri, const std::string& path);
  void set_text(const std::string& text);
  void setup_from_feed(const Feed& feed);
  void set_empty();
  void draw(Graphics* g);

  Button* like_button() {
    return like_button_.get();
  }

  Button* comment_button() {
    return comment_button_.get();
  }

  void open_comment();
  void add_like();
  std::string fold_line(const std::string& line, size_t max_line_length);

 private:

  enum {
    MARGIN = 25,
    IMAGE_MARGIN_TOP = 5,
    IMAGE_HEIGHT = 20,
    IMAGE_WIDTH = 20,
    LIKE_BUTTON_WIDTH = 40,
    LIKE_BUTTON_HEIGHT = 20,
    LIKE_BUTTON_MARGIN_TOP = 2,
    SIDE_BAR_WIDTH = LIKE_BUTTON_WIDTH
  };
  int x_;
  int y_;
  int w_;
  int h_;
  MonAPI::scoped_ptr<Button> like_button_;
  MonAPI::scoped_ptr<Button> comment_button_;
  MonAPI::scoped_ptr<TextField> text_;
  MonAPI::scoped_ptr<ImageIcon> icon_;
  std::string feed_id_;
  int num_likes_;
  int num_comments_;
  Comments comments_;

  DISALLOW_COPY_AND_ASSIGN(FeedView);
};
}

#endif  // CONTRIB_MISC_FACEBOOK_FEED_VIEW_H_