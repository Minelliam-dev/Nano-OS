#include <TFT.h>
#include <SPI.h>

#define cs   10
#define dc   9
#define rst  8

#define XAxis A2
#define YAxis A1
#define Button 6

void (*Reset)(void) = 0;

extern unsigned int __heap_start;
extern void* __brkval;

int FreeRAM() {
    int stackTop;

    return (int)&stackTop -
           (__brkval == 0
                ? (int)&__heap_start
                : (int)__brkval);
}

void DrawGradient(TFT* Screen) {
    for (int y = 0; y < 128; y++) {
        int r = map(y, 0, 127, 10, 80);
        int g = map(y, 0, 127, 20, 10);
        int b = map(y, 0, 127, 50, 120);

        Screen->fillRect(
            0, y,
            160, 1,
            Screen->Color565(r, g, b)
        );
    }
}

class WindowObject {
  public: int X;
  public: int Y;

  public: int SizeX;
  public: int SizeY;

  public: int TextSize = 1;
  public: const char* Text;

  public: int Color;
  
  public: int TextColor;

  public: bool Hovering;

  WindowObject(int x, int y, int Width, int Height, const char* text = "") {
    Y = y;
    X = x;

    SizeX = Width;
    SizeY = Height;

    Text = text;
  };

  public: void Draw(int MX, int MY, int WX, int WY, ::TFT* screen) {
    Hovering = MX >= X+WX && MX <= (X+WX)+SizeX && MY >= Y+WY && MY <= (Y+WY)+SizeY;

    screen->fillRect(X+WX, Y+WY, SizeX, SizeY, Color);
    
    screen->textSize(TextSize);
    screen->stroke(TextColor);
    screen->text(Text, X+WX, Y+WY);
  }
};

class Window {
  public :int Width = 20;
  public :int Height = 20;

  public :int X = 0;
  public :int Y = 0;

  public :TFT* screen = nullptr;

  public :int OffsetX = 0;
  public :int OffsetY = 0;

  public :bool Dragging = false;

  public :bool Terminated = false;

  public :bool Focused = false;
  public :bool JustFocused = false;

  public :int AppID = 0;

  public: int ObjectCount = 0;

  public: void (*UpdateFunction)() = nullptr;
  public: void (*StartFunction)() = nullptr;

  public: uint16_t BG = 43;

  public: bool FirstIteration = true;

  public :WindowObject Objects[10] = {WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0), WindowObject(0, 0, 0, 0)};

  Window(int ID, int x, int y, int width, int height, TFT* Screen, bool focus = false) {
    X = x;
    Y = y;

    Focused = focus;

    AppID = ID;
    
    screen = Screen;

    Width = width;
    Height = height;
  };
  
  void CalculateOffset(int MX, int MY) {
    OffsetX = MX-X;
    OffsetY = MY-Y;
  };

  void UpdateWindowPos(int MX, int MY) {
    X = MX - OffsetX;
    Y = MY - OffsetY;

    DrawGradient(screen);

    Dragging = false;
  };

  public :void DrawWindow(int MouseX, int MouseY, bool Clicking) {
    if (Terminated) return;

    if (FirstIteration) {
      if (StartFunction != nullptr) StartFunction();

      if (BG == 43) BG = screen->Color565(50, 50, 50);

      FirstIteration = false;
    }

    JustFocused = false;
    
    int DragAreaSize = 4;
    uint16_t TitleBarColor = screen->Color565(200, 200, 200);

    bool MouseOnTitleBar = MouseX >= X && MouseX <= X + (Width-DragAreaSize) && MouseY >= Y - DragAreaSize / 2 && MouseY <= Y + DragAreaSize / 2;

    bool MouseOnExitButton = MouseX >= X + (Width-DragAreaSize) && MouseX <= X + Width && MouseY >= Y - DragAreaSize / 2 && MouseY <= Y + DragAreaSize / 2;
    
    if (Clicking && !Dragging && MouseOnTitleBar) {
      CalculateOffset(MouseX, MouseY);
      Dragging = true;

      Focused = true;
      JustFocused = true;
    }
    else if (Dragging && Clicking) {
      UpdateWindowPos(MouseX, MouseY);
    }
    if (Dragging) {
      TitleBarColor = screen->Color565(255, 255, 255);
    }
    if (MouseOnExitButton && Clicking) {
      Terminated = true;
      DrawGradient(screen);

      return;
    }

    //Draw the window with title bar
    screen->fillRect(X, Y-DragAreaSize/2, Width-DragAreaSize, DragAreaSize, TitleBarColor);
    
    screen->fillRect(X+(Width-DragAreaSize), Y-DragAreaSize/2, DragAreaSize, DragAreaSize, screen->Color565(0, 0, 255));
    
    screen->fillRect(X, Y+DragAreaSize/2, Width, Height+(DragAreaSize/2), BG);

    if (UpdateFunction != nullptr) UpdateFunction();

    for (int i=0; i<ObjectCount; i++) {
      Objects[i].Draw(MouseX, MouseY, X, Y+(DragAreaSize/2), screen);
    }
    
  }

  public: void AddObject(WindowObject* Object) {
    if (ObjectCount == 10) {
      return;
    }

    Objects[ObjectCount] = *Object;
    
    ObjectCount++;
  }
};

TFT TFTscreen = TFT(cs, dc, rst);
Window Windows[] = {Window(1, 0, 0, 90, 45, &TFTscreen, true), Window(2, 90, 0, 90, 45, &TFTscreen, false)};

bool WasClicking = false;
bool clicking = false;

int MouseX = 0;
int MouseY = 0;

int JoyStickX = 0;
int JoyStickY = 0;

bool InPopUp = false;

int StartClock = millis();

int UpTimeSec = 0;
int UpTimeTicks = 0;
int UpTimeTickSize = 32756;

bool StartTimerNegative = false;


const int WindowWidth = 160;
const int WindowHeight = 128;


void UpdateTaskManager() {
  if (Windows[1].Terminated) Windows[0].Objects[4].Color = TFTscreen.Color565(0, 0, 255);
  else Windows[0].Objects[4].Color = TFTscreen.Color565(0, 255, 0);

  if (Windows[0].Terminated) Windows[0].Objects[3].Color = TFTscreen.Color565(0, 0, 255);
  else Windows[0].Objects[3].Color = TFTscreen.Color565(0, 255, 0);

  if (Windows[0].Objects[1].Hovering && clicking) {
    Windows[0].Terminated = !Windows[0].Terminated; 
    Windows[0].Objects[3].Text = "Terminated";
    DrawGradient(&TFTscreen);
  }
  else if (!Windows[0].Terminated) Windows[0].Objects[3].Text = "Running";
  
  if (Windows[0].Objects[2].Hovering && clicking) {
    Windows[1].Terminated = !Windows[1].Terminated; 
    Windows[0].Objects[4].Text = "Terminated";
    DrawGradient(&TFTscreen);
  }
  else if (!Windows[1].Terminated) Windows[0].Objects[4].Text = "Running";
}

void TaskManagerStart() {
  Windows[0].AddObject(&WindowObject(0, 0, 0, 0, "Task Manager: "));
  
  Windows[0].AddObject(&WindowObject(0, 12, 25, 10, "ID1"));
  Windows[0].AddObject(&WindowObject(0, 27, 25, 10, "ID2"));

  Windows[0].AddObject(&WindowObject(27, 12, 60, 10, "Running"));
  Windows[0].AddObject(&WindowObject(27, 27, 60, 10, "Running"));

  
  Windows[0].Objects[0].TextColor = TFTscreen.Color565(0, 0, 0);
  
  Windows[0].Objects[1].Color = TFTscreen.Color565(255, 255, 255);
  Windows[0].Objects[2].Color = TFTscreen.Color565(255, 255, 255);
  
  Windows[0].Objects[3].Color = TFTscreen.Color565(0, 255, 0);
  Windows[0].Objects[4].Color = TFTscreen.Color565(0, 255, 0);

  Windows[0].Terminated = true;
}

void TerminalUpdate() {
  if (Windows[1].Objects[0].Hovering && clicking) {
    char* cmd = Input();

    if (cmd == "tst") {
      Windows[1].Objects[1].Text = "tst";
    }

    Serial.println(Input());
  }
}

void TerminalStart() {
  Windows[1].BG = TFTscreen.Color565(0, 0, 0);
  
  Windows[1].AddObject(&WindowObject(0, 0, 40, 10, "C:/ > "));
  Windows[1].Objects[0].TextColor = TFTscreen.Color565(255, 255, 255);
  Windows[1].Objects[0].Color = TFTscreen.Color565(0, 0, 0);

  Windows[1].AddObject(&WindowObject(0, 0, 0, 0, ""));
  Windows[1].Objects[1].TextColor = TFTscreen.Color565(255, 255, 255);
  Windows[1].Objects[1].Color = TFTscreen.Color565(0, 0, 0);
}

int HandleJoyStickX() {
  	int JoyX = 0;

    int Mov = analogRead(XAxis);
    
    if (Mov > 400) {
      JoyX = 1;
    }
    else if (Mov < 200) {
      JoyX = -1;
    }
    else {
      JoyX = 0;
    }

    if (Mov == 501) {
      JoyX = 0;
    }

    return JoyX;
}

int HandleJoyStickY() {
  	int JoyY = 0;

    int Mov = analogRead(YAxis);
    
    if (Mov > 400) {
      JoyY = -1;
    }
    else if (Mov < 200) {
      JoyY = 1;
    }
    else {
      JoyY = 0;
    }

    if (Mov == 516) {
      JoyY = 0;
    }

    return JoyY;
}

void HandleMouse(int X, int Y) {
  int MouseSize = 2;
  
  TFTscreen.fillRect(MouseX, MouseY, MouseSize, MouseSize, 0);
  
  MouseX -= (X*2);
  MouseY -= (Y*2);

  if (MouseX < 0) MouseX = 0;
  if (MouseX > 160) MouseX = 160;

  if (MouseY < 0) MouseY = 0;
  if (MouseY > 128) MouseY = 128;

  TFTscreen.fillRect(MouseX, MouseY, MouseSize, MouseSize, 20);
}

void OpenApp(int ID) {
  if (ID == 1) Windows[0].Terminated = !Windows[0].Terminated;
  else if (ID == 2) Windows[1].Terminated = !Windows[1].Terminated;

  DrawGradient(&TFTscreen);
}

int ShowStartMenu(int BarSize) {
  int MenuHeight = 45;
  
  bool Finished = false;

  int MenuY = 127-((BarSize*2)+MenuHeight);
  int MenuX = 0;

  char* Functions[] = {"Reset", "Tasks", "App 2", "Exit"};

  while (!Finished) {
    bool Clicking = digitalRead(Button) == 0x0;

    bool RequestRedraw = false;
  
    Clicking = Clicking && !WasClicking;

    JoyStickX = HandleJoyStickX();
    JoyStickY = HandleJoyStickY();

    WasClicking = Clicking;
    
    
    TFTscreen.fillRect(MenuX, MenuY, 50, (BarSize+MenuHeight), TFTscreen.Color565(150, 150, 150));

    for (int i=0; i<4; i++) {
      int CurY = MenuY+(i*15);
      bool Hovering = MouseX >= MenuX && MouseX <= MenuX+50 && MouseY >= CurY && MouseY <= CurY+15;
      
      int Color = 100+(Hovering*200);

      TFTscreen.fillRect(MenuX, CurY, 50, 15, TFTscreen.Color565(Color, Color, Color));

      TFTscreen.stroke(0, 0, 0);
      TFTscreen.setTextSize(1);

      TFTscreen.text(Functions[i], MenuX+3, CurY+5);

      if (Hovering && Clicking) {
        return i+1;
      }
    }
  
    HandleMouse(JoyStickX, JoyStickY);
    delay(30);
  }

}

void HandleDesktop(int MX, int MY, bool Clicking) {
  int IconSize = 10;
  int Padding = 2;
  
  int Icons[] = {TFTscreen.Color565(51, 9, 80), TFTscreen.Color565(112, 181, 255)};
  int IconAmount = 2;
  
  TFTscreen.fillRect(0, 128-(IconSize+(Padding*2)), 160, IconSize+(Padding*2), TFTscreen.Color565(150, 150, 150));

  int X = Padding;
  int Y = 127-(IconSize+Padding);

  bool OnStartMenu = MX >= X && MX <= X + IconSize && MY >= Y && MY <= Y + IconSize;

  int ColMult = 1;
  ColMult += (OnStartMenu * ColMult);

  int StartMenuColor = TFTscreen.Color565(62*ColMult, 35+ColMult, 71*ColMult);

  TFTscreen.fillRect(X, Y, IconSize, IconSize, StartMenuColor);

  for (int i=0; i<IconAmount; i++) {
    const int IconOffset = 5+IconSize;
    
    int X = (Padding + 15) + (IconOffset*i);
    int Y = 127-(IconSize+(Padding/2));

    bool OnButton = MX >= X && MX <= X + IconSize && MY >= Y && MY <= Y + IconSize;

    if (OnButton && Clicking && i==0) {
      OpenApp(1);
    }
    if (OnButton && Clicking && i==1) {
      OpenApp(2);
    }
    
    TFTscreen.fillRect(X, Y, IconSize-(Padding), IconSize-(Padding), Icons[i]+(OnButton*200));
  }

  if (OnStartMenu && Clicking) {
    int Selection = ShowStartMenu(IconSize+(Padding*2));

    if (Selection == 1) {
      if (ShowPopUp("Reset?") == 1) {
        Reset();
      }
    }
    else if (Selection == 2) {
      OpenApp(1);
    }
    else if (Selection == 3) {
      OpenApp(2);
    }
    else if (Selection == 4) {
      DrawGradient(&TFTscreen);
    }
  }
}

void DrawWindows(int MX, int MY, bool Clicking) {
  if (InPopUp) return;
  
  int length = sizeof(Windows) / sizeof(Windows[0]);
  
  Window* FocusWindow = nullptr;

  for (int i=0; i<length; i++) {
    if (!Windows[i].Focused) Windows[i].DrawWindow(MX, MY, Clicking);
    else {
      FocusWindow = &Windows[i];
    }

    if (Windows[i].Focused && Windows[i].JustFocused) {
      FocusWindow = &Windows[i];

      for (int w=0; w<length; w++) {
        Windows[w].Focused = false;
        Windows[w].JustFocused = false;
      }

      Windows[i].Focused = true;
    }

  }

  if (FocusWindow != nullptr) FocusWindow->DrawWindow(MX, MY, Clicking);
}

void setup() {

  //D6 = Btn
  //D5 = X
  //D4 = Y

  // Put this line at the beginning of every sketch that uses the GLCD:
  TFTscreen.begin();

  // clear the screen with a black background
  TFTscreen.background(0, 0, 0);

  DrawGradient(&TFTscreen);

  //SecondWindow.Start(90, 50, 80, 45, TFTscreen);

  pinMode(XAxis, INPUT);
  pinMode(YAxis, INPUT);
  pinMode(Button, INPUT);

  Windows[0].UpdateFunction = UpdateTaskManager;
  Windows[0].StartFunction = TaskManagerStart;
  Windows[1].StartFunction = TerminalStart;
  Windows[1].UpdateFunction = TerminalUpdate;


  Serial.begin(9600);
}

int ShowPopUp(char* Text) {
  InPopUp = true;

  DrawGradient(&TFTscreen);

  HandleDesktop(0, 0, false);

  const int PopUpSizeX = 70;
  const int PopUpSizeY = 40;

  const int Padding = 5;

  bool Exited = false;

  int OutputCode = -1;

  while (!Exited) {
    bool Clicking = digitalRead(Button) == 0x0;
  
    Clicking = Clicking && !WasClicking;

    JoyStickX = HandleJoyStickX();
    JoyStickY = HandleJoyStickY();
    
    int X = (WindowWidth/2) - (PopUpSizeX/2);
    int Y = (WindowHeight/2) - (PopUpSizeY/2);

    TFTscreen.fillRect(X, Y, PopUpSizeX, PopUpSizeY, TFTscreen.Color565(300, 300, 300));
  
    TFTscreen.stroke(255, 255, 255);
    TFTscreen.setTextSize(0.5);
    TFTscreen.text(Text, (X*1.5)-15, Y+5);


    int Color1 = TFTscreen.Color565(20, 20, 20);
    int Color2 = TFTscreen.Color565(20, 20, 20);

    int MX = MouseX;
    int MY = MouseY;

    bool OnButton1 = MX >= (X)+(Padding) && MX <= (X)+(Padding) + 30 && MY >= (Y+(PopUpSizeY-(Padding))) - 12 && MY <= (Y+(PopUpSizeY+(Padding)));
    bool OnButton2 = MX >= (X+(Padding))+40 && MX <= (X+(Padding))+40 + 20 && MY >= (Y+(PopUpSizeY-(Padding))) - 12 && MY <= (Y+(PopUpSizeY+(Padding)));

    if (OnButton1) Color1 = TFTscreen.Color565(50, 50, 50);
    if (OnButton2) Color2 = TFTscreen.Color565(50, 50, 50);

    TFTscreen.fillRect((X)+(Padding), (Y+(PopUpSizeY-(Padding)))-8, 30, 12, Color1);
    TFTscreen.text("Yes", (X)+(Padding*2), Y+(PopUpSizeY-(Padding*2)));

    TFTscreen.fillRect((X+(Padding))+40, (Y+(PopUpSizeY-(Padding)))-8, 20, 12, Color2);
    TFTscreen.text("No", (X+(Padding*2)+40), Y+(PopUpSizeY-(Padding*2)));

    HandleMouse(JoyStickX, JoyStickY);
    WasClicking = Clicking;

    if (Clicking && OnButton1) {
      Exited = true;
      InPopUp = false;

      OutputCode = 1;
    }
    else if (Clicking && OnButton2) {
      Exited = true;
      InPopUp = false;

      OutputCode = 2;
    }

    delay(30);
  }
  
  DrawGradient(&TFTscreen);
  return OutputCode;

}

void DrawInputScreen(char* CurrentInput, char* CurrentOutput, bool FirstTime=false) {
  if (FirstTime) TFTscreen.fillRect(0, 0, 160, 128, TFTscreen.Color565(50, 50, 50));
  if (FirstTime) TFTscreen.fillRect(0, WindowHeight/2, 160, WindowHeight/2, TFTscreen.Color565(100, 100, 100));

  int AreaSize = 32;

  TFTscreen.fillRect((WindowWidth/2)-(AreaSize/2), (WindowHeight/2)+20, AreaSize, AreaSize, TFTscreen.Color565(150, 150, 150));

  TFTscreen.stroke(0, 0, 0);
  TFTscreen.setTextSize(3);
  
  TFTscreen.text(CurrentInput, ((WindowWidth/2)-(AreaSize/2))+(AreaSize/4), (WindowHeight/2)+20+(AreaSize/8));
  TFTscreen.text(CurrentOutput, 2, 2);

}

char* Input() {
  String Out = "&";

  bool Finished = false;

  DrawInputScreen("", "", true);

  int CurrentLetterI = 1;

  char* Letters[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "!", "?", "&", "/", ".", ",", "-", "_"};

  const int LetterLength = 43; //Minus 1 because an array starts at 0

  while (!Finished) {
    bool Clicking = digitalRead(Button) == 0x0;

    bool RequestRedraw = false;
  
    Clicking = Clicking && !WasClicking;

    JoyStickX = HandleJoyStickX();
    JoyStickY = HandleJoyStickY();

    //HandleMouse(JoyStickX, JoyStickY);
    WasClicking = Clicking;


    if (JoyStickY == 1) CurrentLetterI += 1;
    else if (JoyStickY == -1) CurrentLetterI -= 1;

    if (CurrentLetterI <= 0) CurrentLetterI = LetterLength;
    else if (CurrentLetterI >= LetterLength) CurrentLetterI = 0;



    if (Clicking) {
      Out += Letters[CurrentLetterI];
    }
    if (JoyStickX == 1) {
      Out.remove(Out.length()-1);
      RequestRedraw = true;
    }
    if (JoyStickX == -1) {
      return Out.c_str();
    }
    
    DrawInputScreen(Letters[CurrentLetterI], Out.c_str(), RequestRedraw);

    delay(60);

  }

  char* OutC = Out.c_str();

  return OutC;
}

void loop() {
  bool Clicking = digitalRead(Button) == 0x0;

  StartClock = millis();

  //Serial.println(StartClock);
  
  Clicking = Clicking && !WasClicking;

  JoyStickX = HandleJoyStickX();
  JoyStickY = HandleJoyStickY();

  DrawWindows(MouseX, MouseY, Clicking);

  HandleDesktop(MouseX, MouseY, Clicking);
  HandleMouse(JoyStickX, JoyStickY);

  WasClicking = Clicking;
  clicking = Clicking;
  
  if (StartClock >= UpTimeTickSize && !StartTimerNegative) {
    UpTimeTicks++;
    StartTimerNegative = true;

    UpTimeSec += 32;
  }
  else if (StartClock >= -30 && StartTimerNegative) {
    UpTimeTicks++;
    StartTimerNegative = false;

    UpTimeSec += 32;
  }

  if (Windows[0].Terminated && Windows[1].Terminated) delay(30);
}


