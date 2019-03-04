/* Simple XCB application drawing a box in a window */

#include <xcb/xcb.h>
#include <xcb/composite.h>
#include <xcb/shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <cairo/cairo-xcb.h>
#include <cstring>

#include <unistd.h> // sleep
#include <list>

#include <xcb/xcb_icccm.h>

using namespace ::std;

struct last_coords
{
    bool is_valid;
    int16_t event_x;
    int16_t event_y;
};

struct coords
{
    int16_t x;
    int16_t y;
    bool connected_with_previous{false};
};

// simple structure to represent some information about our xcb display
// makes it easier to pass certain things around
struct xcb_display_info
{
    // Connection to the X server
    xcb_connection_t *connection{nullptr};
    // Screen
    xcb_screen_t *screen{nullptr};
    // Visual type
    xcb_visualtype_t *visualtype{nullptr};
};

typedef struct xcb_display_info xcb_display_info;

// find a visualtype that supports true transparency
// Check if we can use transparency
xcb_visualtype_t *get_alpha_visualtype(xcb_screen_t *s)
{
    xcb_depth_iterator_t di = xcb_screen_allowed_depths_iterator(s);

    // iterate over the available visualtypes and return the first one with 32bit depth
    for(; di.rem; xcb_depth_next (&di)){
        if(di.data->depth == 32){
            return xcb_depth_visuals_iterator(di.data).data;
        }
    }
    // we didn't find any visualtypes with 32bit depth
    return nullptr;
}

// this will be used as an index when we iterate over the list of atoms
enum {
    NET_WM_WINDOW_TYPE,
    NET_WM_WINDOW_TYPE_DOCK,
    NET_WM_STATE,
    NET_WM_STATE_ABOVE,
    NET_WM_DESKTOP,
    I3_FLOATING_WINDOW
};

// Set the properties of the window
void set_window_properties (xcb_connection_t *connection, xcb_window_t window, char *title)
{
    // atom names that we want to find the atom for
    const char *atom_names[] = {
        "_NET_WM_WINDOW_TYPE",
        "_NET_WM_WINDOW_TYPE_DOCK",
        "_NET_WM_STATE",
        "_NET_WM_STATE_ABOVE",
        "_NET_WM_DESKTOP",
        "I3_FLOATING_WINDOW"
    };

    // get all the atoms
    const int atoms = sizeof(atom_names)/sizeof(char *);
    xcb_intern_atom_cookie_t atom_cookies[atoms];
    xcb_atom_t atom_list[atoms];
    xcb_intern_atom_reply_t *atom_reply;

    for (int i = 0; i < atoms; i++)
        atom_cookies[i] = xcb_intern_atom(connection, 0, strlen(atom_names[i]), atom_names[i]);

    for (int i = 0; i < atoms; i++) {
        atom_reply = xcb_intern_atom_reply(connection, atom_cookies[i], nullptr);
        if (!atom_reply)
        {
            cout << "failed to find atoms" << endl;
            exit(1);
        }
        atom_list[i] = atom_reply->atom;
        free(atom_reply);
    }

    // set the atoms
    const uint32_t desktops[] = {-1U};
   // xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, atom_list[NET_WM_WINDOW_TYPE],
   //         XCB_ATOM_ATOM, 32, 1, &atom_list[NET_WM_WINDOW_TYPE_DOCK]);
    xcb_change_property(connection, XCB_PROP_MODE_APPEND, window, atom_list[NET_WM_STATE],
            XCB_ATOM_ATOM, 32, 2, &atom_list[NET_WM_STATE_ABOVE]);

   // xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, atom_list[NET_WM_DESKTOP],
    //        XCB_ATOM_CARDINAL, 32, 1, desktops);
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING, 8, strlen(title), title);


    //I3_FLOATING_WINDOW
    const uint32_t i3_float[] = {1};
    //xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, atom_list[I3_FLOATING_WINDOW],
           // XCB_ATOM_CARDINAL, 32, 1, i3_float);

    const uint32_t val[] = { 1 };
    xcb_change_window_attributes(connection, window, XCB_CW_OVERRIDE_REDIRECT, val);
    // https://xcb.pdx.freedesktop.narkive.com/eaL6ZK80/xcb-cw-override-redirect-strange-behavior
    // https://www.reddit.com/r/unixporn/comments/5tcs3x/oc_i_created_a_application_launcher_in_c_with_xcb/
}

void clear(xcb_connection_t *connection, cairo_t *cr)
{
    //cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_set_source_rgba(cr, 0, 0, 0,0);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    xcb_flush(connection);
}

void draw(xcb_connection_t *connection, cairo_t *cr, list<coords>& coordinates)
{
    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_set_line_width (cr, 2.5);
    for(std::list<coords>::iterator point = coordinates.begin(); point != coordinates.end(); ++point)
    {
        std::list<coords>::iterator point2 = point;
        ++point2;
        if(point2 != coordinates.end() && point2->connected_with_previous)
        {
            cairo_move_to(cr, point->x, point->y);
            cairo_line_to(cr, point2->x, point2->y);
        }
    }
    cairo_stroke(cr);
    xcb_flush(connection);
}

int
main (int argc, char **argv)
{
  struct xcb_display_info display_info;

  /* open connection with the server */
  // connect to the default display
  display_info.connection = xcb_connect (nullptr, nullptr);

  if (xcb_connection_has_error(display_info.connection) > 0)
    {
      cout <<  "Cannot open display" << endl;
      exit (1);
    }

  //find the default screen
  display_info.screen = xcb_setup_roots_iterator (xcb_get_setup (display_info.connection)).data;
  if(display_info.screen == nullptr)
  {
      cout <<  "Couldn't find the screen" << endl;
      exit (1);
  }

  // get a visualtype with true transparency
  display_info.visualtype = get_alpha_visualtype(display_info.screen);
  if(display_info.visualtype == nullptr)
  {
      cout <<  "Transparency support not found" << endl;
      exit (1);
  }

  /* create windows */
  // generate a colourmap for the window with alpha support
  xcb_window_t colormap = xcb_generate_id(display_info.connection);
  xcb_create_colormap(display_info.connection, XCB_COLORMAP_ALLOC_NONE, colormap,
    display_info.screen->root, display_info.visualtype->visual_id);

  // create the window
  short x{0};
  short y{0};
  unsigned short width{display_info.screen->width_in_pixels};
  unsigned short height{display_info.screen->height_in_pixels};
  unsigned char depth{32U};
  unsigned int dock{0U}; // What is this?
  unsigned short class_val{0U}; // What is this ?
  xcb_window_t window = xcb_generate_id(display_info.connection);
  const uint32_t vals[] = {0x00000000, 0x00000000, dock, XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE, colormap};
  xcb_create_window(display_info.connection, depth, window, display_info.screen->root,
      x, y, width, height, class_val,
      XCB_WINDOW_CLASS_INPUT_OUTPUT, display_info.visualtype->visual_id,
      XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_OVERRIDE_REDIRECT |
          XCB_CW_EVENT_MASK | XCB_CW_COLORMAP,
      vals);

  xcb_free_colormap(display_info.connection, colormap);

  set_window_properties(display_info.connection, window, "smart_pointer");



  // cairo:
  // https://www.cairographics.org/samples/
  // https://lists.cairographics.org/archives/cairo/2017-February/027886.html
  // http://zetcode.com/gfx/cairo/basicdrawing/

  // get a surface for the window and create a destination for it
  cairo_surface_t *window_surface = cairo_xcb_surface_create(display_info.connection, window,
    display_info.visualtype, width, height);

  cairo_surface_t *pointer_surface = cairo_xcb_surface_create(display_info.connection, window,
    display_info.visualtype, width, height);

  cairo_t *cr = cairo_create(window_surface);

  xcb_map_window(display_info.connection, window);

  // some WMs auto-position windows after they are mapped
  // this makes sure it gets to the right place
  const uint32_t values_2[] = {0,0}; // {x,y}
  xcb_configure_window(display_info.connection, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values_2);
  xcb_flush(display_info.connection);




  /* event loop */
  bool done{false};
  bool is_click_on = false;
  last_coords last_coordinates{false, 0, 0};
  list<coords> coordinates;
  xcb_generic_event_t *event;
  bool first_coord{true};
  bool connected_with_previous{false};
  bool something_changed{true};
  while (!done && (event = xcb_wait_for_event (display_info.connection)))
    {
      switch (event->response_type & ~0x80)
        {
        /* (re)draw the window */
        case XCB_EXPOSE:
          cout << "EXPOSE: " << endl;
          break;
      case XCB_BUTTON_PRESS:
      {
            xcb_button_press_event_t *bp = (xcb_button_press_event_t *)event;
            cout << "Button " << (int)bp->detail << " at coordinates (" << bp->event_x << "," <<  bp->event_y << ")" << endl;
            is_click_on = true;
            if(bp->detail == 3)
            {
                done = true;
            }
            if(bp->detail == 2)
            {
                cairo_set_source_surface(cr, window_surface,0, 0);
                clear(display_info.connection, cr);
                something_changed = true;
                coordinates.clear();
                first_coord = true;
            }
            break;
      }
      case XCB_BUTTON_RELEASE: {
         xcb_button_release_event_t *br = (xcb_button_release_event_t *)event;;
        is_click_on = false;
        last_coordinates.is_valid = false;
        connected_with_previous = false;
         cout << "Button " << (int)br->detail << " released at coordinates (" <<br->event_x  << "," << br->event_y << ")d" << endl;
         break;
     }
      case XCB_MOTION_NOTIFY: {
          xcb_motion_notify_event_t *motion = (xcb_motion_notify_event_t *)event;
         // cairo_set_source_surface(cr, pointer_surface,motion->event_x, motion->event_y);
          clear(display_info.connection, cr);
          cout << "Mouse moved at coordinates (" << motion->event_x << "," <<motion->event_y  << ")" << endl;
          cairo_set_source_rgba(cr, 1, 0, 0, 0.5);
          cairo_arc(cr, motion->event_x, motion->event_y, 15, 0, 6.283185);
          cairo_stroke_preserve(cr);
          cairo_set_source_rgba(cr, 0.3, 0.4, 0.6, 0.5);
          cairo_fill(cr);
          xcb_flush(display_info.connection);

          if(is_click_on)
          {
              something_changed = true;
              coordinates.push_back(coords({motion->event_x, motion->event_y, connected_with_previous}));
              connected_with_previous = true;
          }
          if(coordinates.size() > 1 /*&& something_changed == true*/)
          {
               something_changed = false;
               //cairo_set_source_surface(cr, window_surface,motion->event_x, motion->event_y);
              // clear(display_info.connection, cr);
               draw(display_info.connection, cr, coordinates);

          }
          break;
        }
        case XCB_KEY_PRESS:
              cout << "Keycode: " <<  (int)((xcb_key_press_event_t*)event)->detail << endl;
              if(((xcb_key_press_event_t*)event)->detail == 38)
              {
                  done = true;
              }
              break;
        default:
          cout << "Unknown event: " << event->response_type << endl;
          break;
      }
      free (event);
    }

  /* close connection to server */
  xcb_disconnect (display_info.connection);

  return 0;
}
