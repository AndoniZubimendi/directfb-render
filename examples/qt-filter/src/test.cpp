// Sacdo de: http://lists.trolltech.com/qt-interest/2002-06/thread01148-0.html

#include <qapplication.h>
#include <qpushbutton.h>
#include <qdialog.h>
#include <X11/Xlib.h>
#include <iostream>

using namespace std;
class App: public QApplication {
  public:
    App(int argc, char **argv): QApplication(argc, argv) { }
    bool x11EventFilter(XEvent *xe) {
      switch (xe->type) {
        case ButtonPress:
          std::cout << "Caught ButtonPress XEvent" << endl;
	case ButtonRelease:
          std::cout << "Caught ButtonRelease XEvent" << endl;
	case MotionNotify:
          std::cout << "Caught MotionNotify XEvent" << endl;
	case EnterNotify:
          std::cout << "Caught EnterNotify XEvent" << endl;
	case LeaveNotify:
          std::cout << "Caught LeaveNotify XEvent" << endl;
	case FocusIn:
          std::cout << "Caught FocusIn XEvent" << endl;
	case FocusOut: 
          std::cout << "Caught FocusOut XEvent" << endl;
	case KeymapNotify:
          std::cout << "Caught KeymapNotify XEvent" << endl;
	case Expose:
          std::cout << "Caught Expose XEvent" << endl;
	case GraphicsExpose:
          std::cout << "Caught GraphicsExpose XEvent" << endl;
	case NoExpose:
          std::cout << "Caught NoExpose XEvent" << endl;
	case CirculateRequest:
          std::cout << "Caught CirculateRequest XEvent" << endl;
	case ConfigureRequest:
          std::cout << "Caught ConfigureRequest XEvent" << endl;
	case MapRequest:
          std::cout << "Caught MapRequest XEvent" << endl;
	case ResizeRequest:
          std::cout << "Caught ResizeRequest XEvent" << endl;
	case CirculateNotify:
          std::cout << "Caught CirculateNotify XEvent" << endl;
	case ConfigureNotify:
          std::cout << "Caught ConfigureNotify XEvent" << endl;
	case CreateNotify:
          std::cout << "Caught CreateNotify XEvent" << endl;
	case DestroyNotify:
          std::cout << "Caught DestroyNotify XEvent" << endl;
	case GravityNotify:
          std::cout << "Caught GravityNotify XEvent" << endl;
	case MapNotify:
          std::cout << "Caught MapNotify XEvent" << endl;
	case MappingNotify:
          std::cout << "Caught MappingNotify XEvent" << endl;
	case ReparentNotify:
          std::cout << "Caught ReparentNotify XEvent" << endl;
	case UnmapNotify:
          std::cout << "Caught UnmapNotify XEvent" << endl;
	case VisibilityNotify:
          std::cout << "Caught VisibilityNotify XEvent" << endl;
	case ColormapNotify:
          std::cout << "Caught ColormapNotify XEvent" << endl;
	case ClientMessage:
          std::cout << "Caught ClientMessage XEvent" << endl;
	case PropertyNotify:
          std::cout << "Caught PropertyNotify XEvent" << endl;
	case SelectionClear:
          std::cout << "Caught SelectionClear XEvent" << endl;
	case SelectionNotify:
          std::cout << "Caught SelectionNotify XEvent" << endl;
	case SelectionRequest:
          std::cout << "Caught SelectionRequest XEvent" << endl;
        default:
	  std::cout << "Caught event: " << xe->type << endl;
      }
      return false;
    }
    int x11ProcessEvent(XEvent *xe) {
      // nothing here for the time being
    }
};

class Dialog: public QDialog {
  public:
    Dialog(QWidget *parent = 0): QDialog(parent) {
      QPushButton *done = new QPushButton("Done", this);
      connect(done, SIGNAL(clicked()), qApp, SLOT(quit()));
    }
};

int main(int argc, char **argv) {
  App app(argc, argv);
  Dialog dialog;
//  app.setMainWidget(&dialog);
  dialog.show();
  return app.exec();
}
