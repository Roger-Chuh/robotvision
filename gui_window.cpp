/**
 * @author  Hauke Strasdat, Steven Lovegrove
 *
 * Copyright (C) 2010  Hauke Strasdat, Steven Lovegrove
 *                     Imperial College London
 *
 * gui_window.cpp is part of RobotVision.
 *
 * RobotVision is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or any later version.
 *
 * RobotVision is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "gui_window.h"

#include <cvd/gl_helpers.h>
#include <cvd/vector_image_ref.h>

#include "gui_view.h"

using namespace CVD;
using namespace TooN;
using namespace std;
using namespace RobotVision;


GuiWindow::GuiWindow(const ImageRef & ir, const CVD::GLWindow* sharedContext)
  : GLWindow(ir,24,"GuiWindow","" /*,sharedContext*/)
{
  num_id = 0;
  quad = gluNewQuadric();
  active_view = 0;
  mode = 0;
}

GuiWindow::~GuiWindow()
{
  gluDeleteQuadric((GLUquadric* )quad);
}

void GuiWindow::nextFrame(GLWindow::EventHandler& handler)
{
  GLWindow::handle_events(handler);
}

void GuiWindow::handle_events_default() {
  GLWindow::handle_events(*this);
}

void GuiWindow::connectView(AbstractView& view,
                            const RobotVision::Rectangle & box)
{

  RectObj<AbstractView*> ro(box,(&view));
  viewpt_list.push_back(&view);
  view.id = num_id;
  view.bounding_box = box;
  num_id++;
}

AbstractView* GuiWindow::get_view( const CVD::ImageRef& wp, CVD::ImageRef* vp )
{
  for (list<AbstractView*>::iterator iter=viewpt_list.begin();
  iter!=viewpt_list.end();
  ++iter)
  {
    const Rectangle & box = (*iter)->bounding_box;
    const double rel_x = (1.*wp.x) / this->size().x;
    const double rel_y = 1-(1.*wp.y) / this->size().y;
    const ImageRef pixel_size = (*iter)->pixel_size;

    if(box.contains(Rectangle(rel_x, rel_y, rel_x, rel_y))){
      if( vp )
      {
        vp->x = (rel_x-box.x1)/(box.x2-box.x1)*pixel_size.x;
        vp->y = (rel_y-box.y1)/(box.y2-box.y1)*pixel_size.y;
      }
      return *iter;
    }
  }
  return NULL;
}

CVD::ImageRef GuiWindow::view_coords( const AbstractView* view,
                                      const CVD::ImageRef wp ) const
{
  const Rectangle & box = view->bounding_box;
  const double rel_x = (1.*wp.x) / this->size().x;
  const double rel_y = 1-(1.*wp.y) / this->size().y;
  const ImageRef pixel_size = view->pixel_size;
  return ImageRef(
      (rel_x-box.x1)/(box.x2-box.x1)*pixel_size.x,
      (rel_y-box.y1)/(box.y2-box.y1)*pixel_size.y
      );
}


void GuiWindow::set_active_view(AbstractView* view)
{
  if( active_view && active_view->get_handler() )
  {
    active_view->get_handler()->on_event(*this,EVENT_VIEW_DEACTIVATED);
  }

  active_view = view;
  
  if( active_view && active_view->get_handler() )
  {
    active_view->get_handler()->on_event(*this,EVENT_VIEW_ACTIVATED);
  }
}

void GuiWindow::on_key_down(CVD::GLWindow& w, int k)
{
  if( active_view && active_view->get_handler() )
    active_view->get_handler()->on_key_down(w,k);
}

void GuiWindow::on_key_up(CVD::GLWindow& w, int k)
{
  if( active_view && active_view->get_handler() )
    active_view->get_handler()->on_key_up(w,k);
}

void GuiWindow::on_mouse_move(CVD::GLWindow& w,
                              CVD::ImageRef where,
                              int state)
{
  if( active_view && active_view->get_handler() )
  {
    const ImageRef vp = view_coords(active_view,where);
    active_view->get_handler()->on_mouse_move(w,vp,state);
  }
}

void GuiWindow::on_mouse_down(CVD::GLWindow& w,
                              CVD::ImageRef where,
                              int state,
                              int button)
{
  ImageRef vp;
  AbstractView* view = get_view(where,&vp);

  if( view )
  {
    if( view != active_view )
      set_active_view(view);

    if( active_view->get_handler() )
      active_view->get_handler()->on_mouse_down(w,vp,state,button);
  }

  active_pos = cursor_position();
}

void GuiWindow::on_mouse_up(CVD::GLWindow& w,
                            CVD::ImageRef where,
                            int state,
                            int button)
{
  if( active_view && active_view->get_handler() )
  {
    const ImageRef vp = view_coords(active_view,where);
    active_view->get_handler()->on_mouse_up(w,vp,state,button);
  }
}

void GuiWindow::on_resize(CVD::GLWindow& w, CVD::ImageRef size)
{
  if( active_view && active_view->get_handler() )
    active_view->get_handler()->on_resize(w,size);
}

void GuiWindow::on_event(CVD::GLWindow& w, int event)
{
  if( active_view && active_view->get_handler() )
    active_view->get_handler()->on_event(w,event);
}
