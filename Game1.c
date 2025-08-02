#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <math.h>
#include <time.h>

#define MIN(i, j) (((i) < (j)) ? (i) : (j))
#define MAX(i, j) (((i) > (j)) ? (i) : (j))
void Clamp(int* Num_, int Max_) {
    if (*Num_ > Max_) *Num_ = Max_;
}

void HideCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;

    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
}

void GoToXY(int x, int y) {
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void FloatToStr(float Number, char *Buffer, size_t Size) {
    snprintf(Buffer, Size, "%.1f", Number);
}

void IntToStr(int Number, char *Buffer, size_t Size) {
    snprintf(Buffer, Size, "%d", (int)Number);
}

typedef struct {
    int Id;
    int MaxSpeed;
    int Hp;
    float MaxAlt;
    float VisbCoef;
    float SizeCoef;
    float ChanceSpot;
    char TypeName[32];
    char Side[16];
} TargetsLibData;

typedef struct {
    int FileID;
    int UnicueId;
    int Speed;
    float Range;
    float Altitude;
    float VisbCoef;
    float ChanceSpot;
    float Azim;
    float AzimFlight;
    float CoordX, CoordY, CoordZ;
    char TypeName[32];
    char Side[16];
} TargetsData;

#define MaxTypes 64
#define MaxTargets 16
#define TickStep 0.04f
#define RAD2DEG (180.0f / 3.14159265f)

TargetsLibData TargetsMatr[MaxTypes];
TargetsData ActiveTargets[MaxTargets];

int TargetsMatrNum = 0;
int TargetsNum = 0;

int FreeIndex() {
    return TargetsNum;
}

void ObjectDataUpdate() {
    for (int i = 0; i < TargetsNum; i++) {
        ActiveTargets[i].CoordX += cosf(ActiveTargets[i].AzimFlight) * ActiveTargets[i].Speed * 0.001f*TickStep;
        ActiveTargets[i].CoordY += sinf(ActiveTargets[i].AzimFlight) * ActiveTargets[i].Speed * 0.001f*TickStep;
        ActiveTargets[i].Range = sqrtf(ActiveTargets[i].CoordX * ActiveTargets[i].CoordX + ActiveTargets[i].CoordY * ActiveTargets[i].CoordY + ActiveTargets[i].CoordZ * ActiveTargets[i].CoordZ);
        float azim = atan2f(ActiveTargets[i].CoordY, ActiveTargets[i].CoordX) * RAD2DEG;
        ActiveTargets[i].Azim = fmodf(azim + 360.0f, 360.0f);
    }
}

void SpawnObject(int FileIndex, float x, float y, float z, float azimFlight, float speed) {
    
    if (TargetsNum >= MaxTargets) return;

    if (FileIndex < 0 || FileIndex >= TargetsMatrNum) return;

    int idx = FreeIndex();

    ActiveTargets[idx].FileID = FileIndex;
    strncpy(ActiveTargets[idx].TypeName, TargetsMatr[FileIndex].TypeName, sizeof(ActiveTargets[idx].TypeName));
    strncpy(ActiveTargets[idx].Side, TargetsMatr[FileIndex].Side, sizeof(ActiveTargets[idx].Side));

    ActiveTargets[idx].CoordX = x;
    ActiveTargets[idx].CoordY = y;
    ActiveTargets[idx].CoordZ = z;
    ActiveTargets[idx].Altitude = z;
    ActiveTargets[idx].AzimFlight = azimFlight;
    ActiveTargets[idx].Speed = speed;
    ActiveTargets[idx].Range = 20.0f;

    TargetsNum++;
}

void AutoSpawnTarget() {
    if (TargetsNum >= MaxTargets || TargetsMatrNum == 0) return;

    int fileId = rand() % TargetsMatrNum;
    TargetsLibData *src = &TargetsMatr[fileId];

    float x = (float)(rand() % 100 - 50);
    float y = (float)(rand() % 100 - 50);
    float z = src->MaxAlt * ((rand() % 100) / 100.0f);

    float azim = ((float)(rand() % 628)) / 100.0f;
    int speed = (int)(src->MaxSpeed * (0.75f + (rand() % 51) / 100.0f));

    SpawnObject(fileId, x, y, z, azim, speed);
}

#define MaxDistance 30.0f
void RemoveFarTargets() {
    for (int i = 0; i < TargetsNum; ) {
        TargetsData *t = &ActiveTargets[i];

        float dx = t->CoordX;
        float dy = t->CoordY;
        float distance = sqrtf(dx * dx + dy * dy);

        if (distance < MaxDistance) {
            i++;
            continue;
        }

        float vx = cosf(t->AzimFlight) * t->Speed;
        float vy = sinf(t->AzimFlight) * t->Speed;

        float dot = dx * vx + dy * vy;

        if (dot < 0) {
            i++;
            continue;
        }

        for (int j = i; j < TargetsNum - 1; j++) {
            ActiveTargets[j] = ActiveTargets[j + 1];
        }
        TargetsNum--;
    }
}

bool LoadData(const char *FilePath) {
    FILE *File = fopen(FilePath, "r");
    if (!File) {perror("Targets File not found"); return false;}

    char LineData[256];
    fgets(LineData, sizeof(LineData), File);

    while (fgets(LineData, sizeof(LineData), File)) {
        if (TargetsMatrNum >= MaxTypes) break;
        
        TargetsLibData *t = &TargetsMatr[TargetsMatrNum];
        
        int parsed = sscanf(LineData, "%d %d %d %f %f %f %f %s31 %s15", 
            &t->Id, &t->MaxSpeed, &t->Hp, 
            &t->MaxAlt, &t->VisbCoef, &t->SizeCoef, 
            &t->ChanceSpot, t->TypeName, t->Side);
        TargetsMatrNum++;
    }
    fclose(File);
    return true;
}

void DrawData(short TargetNum, short SelectedTarget, float Range, float Altitude, unsigned short Azimut, unsigned short AbsSpeed, const char *Type) {
    char str[32];

    if (TargetNum == SelectedTarget) {
        GoToXY(48, 4+TargetNum);
        printf("%s", ">");
        GoToXY(51, 4+TargetNum);
        printf("%s", "░░┃░       ░┃░               ");
        GoToXY(10, 26);
        IntToStr(TargetNum+1, str, sizeof(str));
        printf("%-2s", str); //TargetNum
        GoToXY(6, 27);
        IntToStr(AbsSpeed, str, sizeof(str));
        printf("%-3s", str); //AbsSpeed
        GoToXY(22, 26);
        FloatToStr(Range, str, sizeof(str));
        printf("%-4s", str); //Range
        GoToXY(22, 27);
        FloatToStr(Altitude, str, sizeof(str));
        printf("%-4s", str); //Altitude
        GoToXY(37, 26);
        printf("%-10s", Type); //Type
        GoToXY(37, 27);
        printf("%s", "    ");
        GoToXY(37, 27);
        IntToStr(Azimut, str, sizeof(str));

        printf("%s°", str); //Azimut
    }
    else {
        GoToXY(48, 4+TargetNum);
        printf(" ");
        GoToXY(51, 4+TargetNum);
        printf("  │         │                ");
    }

    GoToXY(55, 4+TargetNum);
    FloatToStr(Range, str, sizeof(str));
    printf("%s km", str);

    GoToXY(65, 4+TargetNum);
    printf("%s", Type);
}

bool OnDraw(const char *FilePath) {
    FILE *File = fopen(FilePath, "r");
    if (!File) { perror("Ui File not found"); return false; }

    char LineData[512];
    while (fgets(LineData, sizeof(LineData), File)) {
        printf("%s", LineData);
    }
    fclose(File);
    return true;
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    HideCursor();

    if (!OnDraw("UI.ini")) {
        printf("Make sure 'UI.ini' spot in the game directory.\n");
        Sleep(20000); return 1; }
    if (!LoadData("TargetsLib.ini")) {
        printf("Make sure 'TargetsLib.csv' spot in the game directory.\n");
        Sleep(20000); return 1; }
    
    bool GameOver = false;
    unsigned long TickNum = 0ul;
    unsigned short TickTime = 40;
    short SelectedTarget = 0;
    short KeyDelay = 6;
    int ButtonTimer = 0;
    
    while (!GameOver) {
        TickNum++;
        ObjectDataUpdate();

        if (TickNum % 30 == 0) {
            AutoSpawnTarget();
        }
        RemoveFarTargets();

        ButtonTimer++;
        Clamp(&ButtonTimer, KeyDelay + 1);
        if ((GetAsyncKeyState(VK_UP) & 0x8000) && (ButtonTimer > KeyDelay)) {
            SelectedTarget = (SelectedTarget - 1 + TargetsNum) % TargetsNum;
            ButtonTimer = 0;
        }
        else if ((GetAsyncKeyState(VK_DOWN) & 0x8000) && (ButtonTimer > KeyDelay)) {
            SelectedTarget = (SelectedTarget + 1) % TargetsNum;
            ButtonTimer = 0;
        }
        else if (!((GetAsyncKeyState(VK_UP) & 0x8000) || (GetAsyncKeyState(VK_DOWN) & 0x8000))) ButtonTimer = KeyDelay + 1;
        
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            Sleep(2000);
            GameOver = true;
        }

        for (int i = 0; i < TargetsNum; i++) 
        {
            TargetsData *t = &ActiveTargets[i];
            DrawData(
                i,
                SelectedTarget,
                t->Range,
                t->Altitude,
                (unsigned short)t->Azim,
                t->Speed,
                t->TypeName
            );
        }

        Sleep(TickTime);        
    }
    return 0;
}