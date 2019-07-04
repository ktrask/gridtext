#ifndef RECT_BOX_H
#define RECT_BOX_H

#include <Rcpp.h>
using namespace Rcpp;

#include "layout.h"

// A box drawing a rectangle around content
template <class Renderer>
class RectBox : public Box<Renderer> {
private:
  NodePtr m_content;  // any layout node to be placed inside of the rectangle
  Length m_width, m_height; // width and height of the rectangle box (*not the inside*)
  Margin m_margin, m_padding; // margin and padding
  typename Renderer::GraphicsContext m_gp;
  double m_content_hjust, m_content_vjust; // horzontal and vertical justification for content inside the box
  LayoutNode::SizePolicy m_width_policy, m_height_policy; // width and height policies
  Length m_r; // radius of rounded corners
  // position of the box in enclosing box.
  // the box reference point is the leftmost point of the baseline.
  Length m_x, m_y;
  double m_rel_width, m_rel_height; // used to store relative width and height when needed

  // layout calculation when width is defined (doesn't depend on content box)
  void calc_layout_defined_width(Length width_hint, Length height_hint) {
    // width policy is not `native`
    switch(m_width_policy) {
    case LayoutNode::expand:
      m_width = width_hint;
      break;
    case LayoutNode::relative:
      m_width = width_hint * m_rel_width;
      break;
    case LayoutNode::fixed:
    default:
      // nothing to be done for fixed layout, width was set upon creation
      break;
    }

    // if height policy is `native`, we need to layout the content box
    // before deciding on the height
    if (m_height_policy == LayoutNode::native) {
      if (m_content == nullptr) {
        // content is empty, nothing to layout
        m_height = m_margin.top + m_margin.bottom + m_padding.top + m_padding.bottom;
      } else {
        Length content_width_hint = m_width - m_margin.left - m_margin.right - m_padding.left - m_padding.right;
        Length content_height_hint = height_hint - m_margin.top - m_margin.bottom - m_padding.top - m_padding.bottom;
        auto b = static_pointer_cast<Box<Renderer> >(m_content);
        b->calc_layout(content_width_hint, content_height_hint);
        m_height = b->height() + m_margin.top + m_margin.bottom + m_padding.top + m_padding.bottom;
        b->place(m_margin.left + m_padding.left, m_margin.bottom + m_padding.bottom + b->descent() - b->voff());
      }
    } else {
      // height is also defined
     switch(m_height_policy) {
      case LayoutNode::expand:
        m_height = height_hint;
        break;
      case LayoutNode::relative:
        m_height = height_hint * m_rel_height;
        break;
      case LayoutNode::fixed:
      default:
        // nothing to be done for fixed layout, height was set upon creation
        break;
      }

      // if we have content, we still need to layout it
      if (!(m_content == nullptr)) {
        Length content_width_hint = m_width - m_margin.left - m_margin.right - m_padding.left - m_padding.right;
        Length content_height_hint = m_height - m_margin.top - m_margin.bottom - m_padding.top - m_padding.bottom;
        auto b = static_pointer_cast<Box<Renderer> >(m_content);
        b->calc_layout(content_width_hint, content_height_hint);
        b->place(m_margin.left + m_padding.left, m_margin.bottom + m_padding.bottom + b->descent() - b->voff());
      }
    }
  }

  // layout calculation when width is native
  void calc_layout_native_width(Length width_hint, Length height_hint) {
    // if height policy is `native`, we need to layout the content box
    // before deciding on the height
    if (m_height_policy == LayoutNode::native) {
      if (m_content == nullptr) {
        // content is empty, nothing to layout
        m_width = m_margin.left + m_margin.right + m_padding.left + m_padding.right;
        m_height = m_margin.top + m_margin.bottom + m_padding.top + m_padding.bottom;
      } else {
        Length content_width_hint = width_hint - m_margin.left - m_margin.right - m_padding.left - m_padding.right;
        Length content_height_hint = height_hint - m_margin.top - m_margin.bottom - m_padding.top - m_padding.bottom;
        auto b = static_pointer_cast<Box<Renderer> >(m_content);
        b->calc_layout(content_width_hint, content_height_hint);
        m_width = b->width() + m_margin.right + m_padding.left + m_padding.right;
        m_height = b->height() + m_margin.bottom + m_padding.top + m_padding.bottom;
        b->place(m_margin.left + m_padding.left, m_margin.bottom + m_padding.bottom + b->descent() - b->voff());
      }
    } else {
      // height is defined
      switch(m_height_policy) {
      case LayoutNode::expand:
        m_height = height_hint;
        break;
      case LayoutNode::relative:
        m_height = height_hint * m_rel_height;
        break;
      case LayoutNode::fixed:
      default:
        // nothing to be done for fixed layout, height was set upon creation
        break;
      }

      // height is set, but we still need to set width
      if (m_content == nullptr) {
        // content is empty, nothing to layout
        m_width = m_margin.left + m_margin.right + m_padding.left + m_padding.right;
      } else {
        Length content_width_hint = width_hint - m_margin.left - m_margin.right - m_padding.left - m_padding.right;
        Length content_height_hint = m_height - m_margin.top - m_margin.bottom - m_padding.top - m_padding.bottom;
        auto b = static_pointer_cast<Box<Renderer> >(m_content);
        b->calc_layout(content_width_hint, content_height_hint);
        m_width = b->width() + m_margin.right + m_padding.left + m_padding.right;
        b->place(m_margin.left + m_padding.left, m_margin.bottom + m_padding.bottom + b->descent() - b->voff());
      }
    }
  }


public:
  RectBox(const NodePtr &content,
          Length width, Length height,
          const Margin &margin, const Margin &padding,
          const typename Renderer::GraphicsContext &gp,
          double content_hjust = 0, double content_vjust = 1,
          LayoutNode::SizePolicy width_policy = LayoutNode::native,
          LayoutNode::SizePolicy height_policy = LayoutNode::native,
          Length r = 0) :
    m_content(content), m_width(width), m_height(height), m_margin(margin), m_padding(padding),
    m_gp(gp), m_content_hjust(content_hjust), m_content_vjust(content_vjust),
    m_width_policy(width_policy), m_height_policy(height_policy),
    m_r(r), m_x(0), m_y(0), m_rel_width(0), m_rel_height(0) {
    // save relative width and height if needed
    if (m_width_policy == LayoutNode::relative) {
      m_rel_width = m_width/100;
    }
    if (m_height_policy == LayoutNode::relative) {
      m_rel_height = m_height/100;
    }
  }
  ~RectBox() {};

  Length width() { return m_width; }
  Length ascent() { return m_height; }
  Length descent() { return 0; }
  Length voff() { return 0; }

  void calc_layout(Length width_hint, Length height_hint) {
    if (m_width_policy == LayoutNode::native) {
      calc_layout_native_width(width_hint, height_hint);
    } else {
      calc_layout_defined_width(width_hint, height_hint);
    }
  }

  // place box in internal coordinates used in enclosing box
  void place(Length x, Length y) {
    m_x = x;
    m_y = y;
  }

  // render into absolute coordinates, using the reference coordinates
  // from the enclosing box
  void render(Renderer &r, Length xref, Length yref) {
    Length x = m_x + xref + m_margin.left;
    Length y = m_y + yref + m_margin.bottom;
    Length width = m_width - m_margin.left - m_margin.right;
    Length height = m_height - m_margin.bottom - m_margin.top;

    r.rect(x, y, width, height, m_gp, m_r);

    // if we have content we need to render it
    if (!(m_content == nullptr)) {
      auto b = static_pointer_cast<Box<Renderer> >(m_content);
      b->render(r, xref, yref);
    }
  }
};

#endif