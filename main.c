#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define MAX 1000
#define NAME_LEN 100
#define PATIENT_FILE "patients.txt"
#define HISTORY_FILE "treated_history.txt"
#define DOCTOR_FILE "doctors.txt"
#define BILL_FILE "billing.txt"

#define RED     "\033[1;31m"
#define GREEN   "\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE    "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN    "\033[1;36m"
#define WHITE   "\033[1;37m"
#define RESET   "\033[0m"

typedef struct {
    int id;
    char name[NAME_LEN];
    int age, emergency, arrival, doctorId;
    char doctorName[NAME_LEN];
} Patient;

typedef struct {
    int id;
    char name[NAME_LEN], specialization[NAME_LEN], phone[30];
    int room, available;
    double visitFee;
} Doctor;

typedef struct {
    int patientId;
    char patientName[NAME_LEN];
    int doctorId;
    char doctorName[NAME_LEN];
    double billAmount, paidAmount;
    char date[20];
} Bill;

Patient patients[MAX], treated[MAX];
Doctor doctors[MAX];
Bill bills[MAX];
int patientCount = 0, treatedCount = 0, doctorCount = 0, billCount = 0;
int arrivalCount = 0;

void showTitle(void);
void showMenu(void);
void showClock(void);
void startLiveClock(void);
void stopLiveClock(void);
int readInt(const char *prompt);
void readText(const char *prompt, char *text, int size);
void pauseScreen(void);
void saveData(void);
void loadData(void);
void saveBills(void);
void loadBills(void);
void addPatient(void);
void displayPatients(void);
void sortPatients(void);
void treatPatient(void);
void searchPatient(void);
void deletePatient(void);
void updatePatient(void);
void showHistory(void);
void addDoctor(void);
void displayDoctors(void);
void searchDoctor(void);
void updateDoctor(void);
void deleteDoctor(void);
void showBillingReport(void);
int adminLogin(void);
int patientIdExists(int id);
int doctorIdExists(int id);
Doctor *findDoctor(int id);
const char *priorityName(int p);

#ifdef _WIN32
static HANDLE liveClockThread = NULL;
static volatile LONG liveClockRunning = 0;
static SHORT liveClockRow = 4;

static DWORD WINAPI liveClockWorker(LPVOID unused)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    (void)unused;

    while (InterlockedCompareExchange(&liveClockRunning, 1, 1) == 1) {
        time_t now = time(NULL);
        struct tm *tmNow = localtime(&now);
        char line[71];
        DWORD written;
        WORD attrs[70];
        COORD pos = {0, liveClockRow};
        int i;

        if (tmNow != NULL) {
            snprintf(line, sizeof(line),
                     "          LIVE TIME: %02d:%02d:%02d   DATE: %02d-%02d-%04d",
                     tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec,
                     tmNow->tm_mday, tmNow->tm_mon + 1,
                     tmNow->tm_year + 1900);

            for (i = (int)strlen(line); i < 70; ++i) line[i] = ' ';
            line[70] = '\0';

            for (i = 0; i < 70; ++i)
                attrs[i] = FOREGROUND_GREEN | FOREGROUND_BLUE |
                           FOREGROUND_INTENSITY;

            WriteConsoleOutputCharacterA(out, line, 70, pos, &written);
            WriteConsoleOutputAttribute(out, attrs, 70, pos, &written);
        }
        Sleep(1000);
    }
    return 0;
}
#endif

void startLiveClock(void)
{
#ifdef _WIN32
    if (liveClockThread != NULL) return;
    InterlockedExchange(&liveClockRunning, 1);
    liveClockThread = CreateThread(NULL, 0, liveClockWorker, NULL, 0, NULL);
#endif
}

void stopLiveClock(void)
{
#ifdef _WIN32
    InterlockedExchange(&liveClockRunning, 0);
    if (liveClockThread != NULL) {
        WaitForSingleObject(liveClockThread, INFINITE);
        CloseHandle(liveClockThread);
        liveClockThread = NULL;
    }
#endif
}

void showClock(void)
{
    time_t now = time(NULL);
    struct tm *tmNow = localtime(&now);
#ifdef _WIN32
    liveClockRow = 5;
#endif
    if (tmNow != NULL)
        printf(CYAN "          LIVE TIME: %02d:%02d:%02d   DATE: %02d-%02d-%04d\n" RESET,
               tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec,
               tmNow->tm_mday, tmNow->tm_mon + 1, tmNow->tm_year + 1900);
}

int readInt(const char *prompt)
{
    char buf[100], *end;
    long value;
    for (;;) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof(buf), stdin)) return 0;
        value = strtol(buf, &end, 10);
        while (isspace((unsigned char)*end)) ++end;
        if (end != buf && *end == '\0' &&
            value >= -2147483647L - 1 && value <= 2147483647L)
            return (int)value;
        printf(RED "Invalid input! Enter a number.\n" RESET);
    }
}

void readText(const char *prompt, char *text, int size)
{
    for (;;) {
        char *start;
        int len;
        printf("%s", prompt);
        if (!fgets(text, size, stdin)) { text[0] = '\0'; return; }
        text[strcspn(text, "\n")] = '\0';
        start = text;
        while (isspace((unsigned char)*start)) ++start;
        if (start != text) memmove(text, start, strlen(start) + 1);
        len = (int)strlen(text);
        while (len > 0 && isspace((unsigned char)text[len - 1]))
            text[--len] = '\0';
        if (len == 0) {
            printf(RED "Input cannot be empty!\n" RESET);
            continue;
        }
        if (strchr(text, '|')) {
            printf(RED "The character | is not allowed.\n" RESET);
            continue;
        }
        return;
    }
}

void pauseScreen(void)
{
    char temp[10];
    printf(YELLOW "\nPress Enter to continue..." RESET);
    fgets(temp, sizeof(temp), stdin);
}

const char *priorityName(int p)
{
    if (p == 1) return "CRITICAL";
    if (p == 2) return "SERIOUS";
    if (p == 3) return "NORMAL";
    return "UNKNOWN";
}

void showTitle(void)
{
    printf(CYAN "====================================================\n");
    printf("          HOSPITAL PATIENT MANAGEMENT SYSTEM\n");
    printf("                         MADE BY\n");
    printf("             TANJIUL RAIHAN ****** S*** ***\n");
    printf("====================================================\n" RESET);
    showClock();
    printf(CYAN "====================================================\n" RESET);
}

void showMenu(void)
{
    printf(BLUE "\n--------------- MAIN MENU ----------------\n" RESET);
    printf(WHITE "1. Add New Patient\n2. Display Waiting Patients\n");
    printf("3. Sort by Emergency Priority\n4. Treat Next Patient\n");
    printf("5. Search Patient\n6. Delete Waiting Patient\n");
    printf("7. Update Patient Information\n8. View Treated History\n");
    printf("9. Save Data\n10. Add New Doctor\n11. Display Doctors\n");
    printf("12. Search Doctor\n13. Update Doctor\n14. Delete Doctor\n");
    printf("15. Billing and Income Report\n0. Exit Program\n" RESET);
}

int patientIdExists(int id)
{
    int i;
    for (i = 0; i < patientCount; ++i) if (patients[i].id == id) return 1;
    for (i = 0; i < treatedCount; ++i) if (treated[i].id == id) return 1;
    return 0;
}

int doctorIdExists(int id)
{
    int i;
    for (i = 0; i < doctorCount; ++i) if (doctors[i].id == id) return 1;
    return 0;
}

Doctor *findDoctor(int id)
{
    int i;
    for (i = 0; i < doctorCount; ++i) if (doctors[i].id == id) return &doctors[i];
    return NULL;
}

void saveBills(void)
{
    FILE *fp = fopen(BILL_FILE, "w");
    int i;
    if (!fp) { printf(RED "Could not save billing data!\n" RESET); return; }
    fprintf(fp, "%d\n", billCount);
    for (i = 0; i < billCount; ++i)
        fprintf(fp, "%d|%s|%d|%s|%.2f|%.2f|%s\n",
                bills[i].patientId, bills[i].patientName, bills[i].doctorId,
                bills[i].doctorName, bills[i].billAmount, bills[i].paidAmount,
                bills[i].date);
    fclose(fp);
}

void loadBills(void)
{
    FILE *fp = fopen(BILL_FILE, "r");
    char line[1200];
    int count, i;
    billCount = 0;
    if (!fp) return;
    if (fscanf(fp, "%d\n", &count) != 1) { fclose(fp); return; }
    if (count < 0) count = 0;
    if (count > MAX) count = MAX;
    for (i = 0; i < count; ++i) {
        Bill b;
        if (!fgets(line, sizeof(line), fp)) break;
        if (sscanf(line, "%d|%99[^|]|%d|%99[^|]|%lf|%lf|%19[^\n]",
                   &b.patientId, b.patientName, &b.doctorId, b.doctorName,
                   &b.billAmount, &b.paidAmount, b.date) == 7)
            bills[billCount++] = b;
    }
    fclose(fp);
}

void saveData(void)
{
    FILE *fp;
    int i;
    fp = fopen(PATIENT_FILE, "w");
    if (!fp) { printf(RED "Cannot save patient data!\n" RESET); return; }
    fprintf(fp, "%d %d\n", arrivalCount, patientCount);
    for (i = 0; i < patientCount; ++i)
        fprintf(fp, "%d|%s|%d|%d|%d|%d|%s\n", patients[i].id,
                patients[i].name, patients[i].age, patients[i].emergency,
                patients[i].arrival, patients[i].doctorId, patients[i].doctorName);
    fclose(fp);

    fp = fopen(DOCTOR_FILE, "w");
    if (!fp) { printf(RED "Cannot save doctor data!\n" RESET); return; }
    fprintf(fp, "%d\n", doctorCount);
    for (i = 0; i < doctorCount; ++i)
        fprintf(fp, "%d|%s|%s|%s|%d|%d|%.2f\n", doctors[i].id, doctors[i].name,
                doctors[i].specialization, doctors[i].phone, doctors[i].room,
                doctors[i].available, doctors[i].visitFee);
    fclose(fp);

    fp = fopen(HISTORY_FILE, "w");
    if (!fp) { printf(RED "Cannot save treatment history!\n" RESET); return; }
    fprintf(fp, "%d\n", treatedCount);
    for (i = 0; i < treatedCount; ++i)
        fprintf(fp, "%d|%s|%d|%d|%d|%d|%s\n", treated[i].id, treated[i].name,
                treated[i].age, treated[i].emergency, treated[i].arrival,
                treated[i].doctorId, treated[i].doctorName);
    fclose(fp);
    saveBills();
    printf(GREEN "Data saved successfully.\n" RESET);
}

void loadData(void)
{
    FILE *fp;
    char line[1200];
    int count, i;
    patientCount = treatedCount = doctorCount = arrivalCount = 0;

    fp = fopen(PATIENT_FILE, "r");
    if (fp) {
        if (fscanf(fp, "%d %d\n", &arrivalCount, &count) == 2) {
            if (count < 0) count = 0; if (count > MAX) count = MAX;
            for (i = 0; i < count && fgets(line, sizeof(line), fp); ++i) {
                Patient p;
                if (sscanf(line, "%d|%99[^|]|%d|%d|%d|%d|%99[^\n]",
                    &p.id, p.name, &p.age, &p.emergency, &p.arrival,
                    &p.doctorId, p.doctorName) == 7 && p.id > 0 &&
                    p.age >= 0 && p.emergency >= 1 && p.emergency <= 3 &&
                    patientCount < MAX)
                    patients[patientCount++] = p;
            }
        }
        fclose(fp);
    }

    fp = fopen(HISTORY_FILE, "r");
    if (fp) {
        if (fscanf(fp, "%d\n", &count) == 1) {
            if (count < 0) count = 0; if (count > MAX) count = MAX;
            for (i = 0; i < count && fgets(line, sizeof(line), fp); ++i) {
                Patient p;
                if (sscanf(line, "%d|%99[^|]|%d|%d|%d|%d|%99[^\n]",
                    &p.id, p.name, &p.age, &p.emergency, &p.arrival,
                    &p.doctorId, p.doctorName) == 7 && p.id > 0 &&
                    p.age >= 0 && p.emergency >= 1 && p.emergency <= 3 &&
                    treatedCount < MAX)
                    treated[treatedCount++] = p;
                if (p.arrival > arrivalCount) arrivalCount = p.arrival;
            }
        }
        fclose(fp);
    }

    fp = fopen(DOCTOR_FILE, "r");
    if (fp) {
        if (fscanf(fp, "%d\n", &count) == 1) {
            if (count < 0) count = 0; if (count > MAX) count = MAX;
            for (i = 0; i < count && fgets(line, sizeof(line), fp); ++i) {
                Doctor d;
                if (sscanf(line, "%d|%99[^|]|%99[^|]|%29[^|]|%d|%d|%lf",
                    &d.id, d.name, d.specialization, d.phone, &d.room,
                    &d.available, &d.visitFee) == 7 && d.id > 0 &&
                    d.room > 0 && (d.available == 0 || d.available == 1) &&
                    doctorCount < MAX)
                    doctors[doctorCount++] = d;
            }
        }
        fclose(fp);
    }
    for (i = 0; i < patientCount; ++i)
        if (patients[i].arrival > arrivalCount) arrivalCount = patients[i].arrival;
    loadBills();
    printf(GREEN "Saved data loaded successfully.\n" RESET);
}

void addDoctor(void)
{
    Doctor d;
    if (doctorCount >= MAX) { puts("Doctor list is full."); return; }
    d.id = readInt("Enter Doctor ID: ");
    if (d.id <= 0 || doctorIdExists(d.id)) { puts(RED "Invalid or duplicate ID." RESET); return; }
    readText("Enter Doctor Name: ", d.name, sizeof(d.name));
    readText("Enter Specialization: ", d.specialization, sizeof(d.specialization));
    readText("Enter Phone Number: ", d.phone, sizeof(d.phone));
    d.room = readInt("Enter Room Number: ");
    d.visitFee = readInt("Enter Doctor Visit Fee (Taka): ");
    d.available = readInt("Available? (1=yes, 0=no): ");
    if (d.room <= 0 || d.visitFee < 0 || (d.available != 0 && d.available != 1)) {
        puts(RED "Invalid room, fee, or status." RESET); return;
    }
    doctors[doctorCount++] = d; saveData();
    puts(GREEN "Doctor added successfully." RESET);
}

void displayDoctors(void)
{
    int i;
    printf(CYAN "\n--- DOCTORS ---\n" RESET);
    if (!doctorCount) { puts("No doctors available."); return; }
    printf("%-5s %-20s %-20s %-15s %-6s %-12s %-10s\n",
           "ID","NAME","SPECIALIZATION","PHONE","ROOM","STATUS","FEE");
    for (i = 0; i < doctorCount; ++i)
        printf("%-5d %-20.20s %-20.20s %-15.15s %-6d %-12s %.2f\n",
               doctors[i].id, doctors[i].name, doctors[i].specialization,
               doctors[i].phone, doctors[i].room,
               doctors[i].available ? "AVAILABLE" : "UNAVAILABLE",
               doctors[i].visitFee);
}

void searchDoctor(void)
{
    int id = readInt("Enter Doctor ID: ");
    Doctor *d = findDoctor(id);
    if (!d) { puts(RED "Doctor not found." RESET); return; }
    printf("ID: %d\nName: %s\nSpecialization: %s\nPhone: %s\nRoom: %d\nFee: %.2f\nStatus: %s\n",
           d->id, d->name, d->specialization, d->phone, d->room, d->visitFee,
           d->available ? "Available" : "Unavailable");
}

void updateDoctor(void)
{
    int id = readInt("Enter Doctor ID to update: ");
    Doctor *d = findDoctor(id);
    if (!d) { puts(RED "Doctor not found." RESET); return; }
    readText("New name: ", d->name, sizeof(d->name));
    readText("New specialization: ", d->specialization, sizeof(d->specialization));
    readText("New phone: ", d->phone, sizeof(d->phone));
    d->room = readInt("New room: ");
    d->visitFee = readInt("New visit fee: ");
    d->available = readInt("Available? (1/0): ");
    if (d->room <= 0 || d->visitFee < 0 || (d->available != 0 && d->available != 1)) {
        puts(RED "Invalid details; update not saved." RESET); return;
    }
    saveData(); puts(GREEN "Doctor updated." RESET);
}

void deleteDoctor(void)
{
    int id = readInt("Enter Doctor ID to delete: "), i, j;
    char confirm[10];
    for (i = 0; i < doctorCount; ++i) if (doctors[i].id == id) {
        for (j = 0; j < patientCount; ++j)
            if (patients[j].doctorId == id) { puts("Doctor assigned to a waiting patient."); return; }
        for (j = 0; j < treatedCount; ++j)
            if (treated[j].doctorId == id) { puts("Doctor exists in treatment history."); return; }
        readText("Type yes to confirm: ", confirm, sizeof(confirm));
        if (strcmp(confirm, "yes") && strcmp(confirm, "YES") && strcmp(confirm, "Yes")) {
            puts("Cancelled."); return;
        }
        for (j = i; j < doctorCount - 1; ++j) doctors[j] = doctors[j + 1];
        --doctorCount; saveData(); puts(GREEN "Doctor deleted." RESET); return;
    }
    puts(RED "Doctor not found." RESET);
}

void addPatient(void)
{
    Patient p;
    Doctor *d;
    if (patientCount >= MAX) { puts("Waiting list is full."); return; }
    p.id = readInt("Enter Patient ID: ");
    if (p.id <= 0 || patientIdExists(p.id)) { puts(RED "Invalid or duplicate ID." RESET); return; }
    readText("Enter Patient Name: ", p.name, sizeof(p.name));
    p.age = readInt("Enter Patient Age: ");
    p.emergency = readInt("Priority (1 Critical, 2 Serious, 3 Normal): ");
    if (p.age < 0 || p.age > 130 || p.emergency < 1 || p.emergency > 3) {
        puts(RED "Invalid age or priority." RESET); return;
    }
    displayDoctors();
    p.doctorId = readInt("Enter Doctor ID: ");
    d = findDoctor(p.doctorId);
    if (!d || !d->available) { puts(RED "Doctor not found or unavailable." RESET); return; }
    strcpy(p.doctorName, d->name);
    p.arrival = ++arrivalCount;
    patients[patientCount++] = p; saveData();
    puts(GREEN "Patient added successfully." RESET);
}

void displayPatients(void)
{
    int i;
    printf(CYAN "\n--- WAITING PATIENTS ---\n" RESET);
    if (!patientCount) { puts("No waiting patients."); return; }
    printf("%-6s %-20s %-5s %-10s %-20s %-7s\n","ID","NAME","AGE","PRIORITY","DOCTOR","ARRIVAL");
    for (i = 0; i < patientCount; ++i)
        printf("%-6d %-20.20s %-5d %-10s %-20.20s %-7d\n",
               patients[i].id, patients[i].name, patients[i].age,
               priorityName(patients[i].emergency), patients[i].doctorName,
               patients[i].arrival);
}

void sortPatients(void)
{
    int i, j;
    Patient temp;
    for (i = 0; i < patientCount - 1; ++i)
        for (j = 0; j < patientCount - i - 1; ++j)
            if (patients[j].emergency > patients[j+1].emergency ||
               (patients[j].emergency == patients[j+1].emergency &&
                patients[j].arrival > patients[j+1].arrival)) {
                temp = patients[j]; patients[j] = patients[j+1]; patients[j+1] = temp;
            }
    saveData();
}

void treatPatient(void)
{
    char confirm[10];
    int i;
    Bill b;
    time_t now;
    struct tm *tmNow;
    if (!patientCount) { puts("No waiting patients."); return; }
    if (treatedCount >= MAX || billCount >= MAX) { puts("History or billing list is full."); return; }
    sortPatients();
    printf("Next patient: ID %d, %s, priority %s, doctor %s\n",
           patients[0].id, patients[0].name, priorityName(patients[0].emergency),
           patients[0].doctorName);
    readText("Confirm treatment? (yes/no): ", confirm, sizeof(confirm));
    if (strcmp(confirm,"yes") && strcmp(confirm,"YES") && strcmp(confirm,"Yes")) {
        puts("Treatment cancelled."); return;
    }
    b.patientId = patients[0].id; strcpy(b.patientName, patients[0].name);
    b.doctorId = patients[0].doctorId; strcpy(b.doctorName, patients[0].doctorName);
    b.billAmount = 0;
    for (i = 0; i < doctorCount; ++i)
        if (doctors[i].id == b.doctorId) { b.billAmount = doctors[i].visitFee; break; }
    printf("Doctor visit fee: %.2f Taka\n", b.billAmount);
    b.paidAmount = readInt("Enter amount received now (Taka): ");
    if (b.paidAmount < 0) b.paidAmount = 0;
    if (b.paidAmount > b.billAmount) b.paidAmount = b.billAmount;
    now = time(NULL); tmNow = localtime(&now);
    if (tmNow) strftime(b.date, sizeof(b.date), "%Y-%m-%d", tmNow);
    else strcpy(b.date, "0000-00-00");
    bills[billCount++] = b;
    treated[treatedCount++] = patients[0];
    for (i = 0; i < patientCount - 1; ++i) patients[i] = patients[i+1];
    --patientCount; saveData();
    puts(GREEN "Treatment and billing recorded." RESET);
}

void searchPatient(void)
{
    int id = readInt("Enter Patient ID: "), i, found = 0;
    for (i = 0; i < patientCount; ++i) if (patients[i].id == id) {
        printf("WAITING: ID %d | %s | Age %d | %s | Doctor %s\n", patients[i].id,
               patients[i].name, patients[i].age, priorityName(patients[i].emergency),
               patients[i].doctorName); found = 1;
    }
    for (i = 0; i < treatedCount; ++i) if (treated[i].id == id) {
        printf("TREATED: ID %d | %s | Age %d | %s | Doctor %s\n", treated[i].id,
               treated[i].name, treated[i].age, priorityName(treated[i].emergency),
               treated[i].doctorName); found = 1;
    }
    if (!found) puts(RED "Patient not found." RESET);
}

void deletePatient(void)
{
    int id = readInt("Enter waiting Patient ID to delete: "), i, j;
    char confirm[10];
    for (i = 0; i < patientCount; ++i) if (patients[i].id == id) {
        readText("Type yes to confirm deletion: ", confirm, sizeof(confirm));
        if (strcmp(confirm,"yes") && strcmp(confirm,"YES") && strcmp(confirm,"Yes")) {
            puts("Cancelled."); return;
        }
        for (j = i; j < patientCount - 1; ++j) patients[j] = patients[j+1];
        --patientCount; saveData(); puts(GREEN "Patient deleted." RESET); return;
    }
    puts(RED "Waiting patient not found." RESET);
}

void updatePatient(void)
{
    int id = readInt("Enter waiting Patient ID to update: "), i, age, priority, did;
    char name[NAME_LEN];
    Doctor *d;
    for (i = 0; i < patientCount; ++i) if (patients[i].id == id) {
        readText("New name: ", name, sizeof(name));
        age = readInt("New age: ");
        priority = readInt("New priority (1 Critical, 2 Serious, 3 Normal): ");
        if (age < 0 || age > 130 || priority < 1 || priority > 3) {
            puts(RED "Invalid age or priority." RESET); return;
        }
        displayDoctors(); did = readInt("New Doctor ID: "); d = findDoctor(did);
        if (!d || !d->available) { puts(RED "Doctor not found or unavailable." RESET); return; }
        strcpy(patients[i].name, name); patients[i].age = age;
        patients[i].emergency = priority; patients[i].doctorId = d->id;
        strcpy(patients[i].doctorName, d->name); saveData();
        puts(GREEN "Patient updated." RESET); return;
    }
    puts(RED "Waiting patient not found." RESET);
}

void showHistory(void)
{
    int i;
    printf(MAGENTA "\n--- TREATED PATIENT HISTORY ---\n" RESET);
    if (!treatedCount) { puts("No history."); return; }
    for (i = 0; i < treatedCount; ++i)
        printf("ID: %d | Name: %s | Age: %d | Priority: %s | Doctor: %s | Arrival: %d\n",
               treated[i].id, treated[i].name, treated[i].age,
               priorityName(treated[i].emergency), treated[i].doctorName,
               treated[i].arrival);
}

void showBillingReport(void)
{
    int i, choice;
    double totalBill = 0, totalPaid = 0, selected = 0;
    char today[20], month[8], year[5];
    time_t now = time(NULL);
    struct tm *tmNow = localtime(&now);
    if (!tmNow) return;
    strftime(today, sizeof(today), "%Y-%m-%d", tmNow);
    strftime(month, sizeof(month), "%Y-%m", tmNow);
    strftime(year, sizeof(year), "%Y", tmNow);
    puts("\n1. Today's income\n2. This month's income\n3. This year's income\n4. All-time income");
    choice = readInt("Choose report: ");
    for (i = 0; i < billCount; ++i) {
        totalBill += bills[i].billAmount; totalPaid += bills[i].paidAmount;
        if (choice == 1 && strncmp(bills[i].date, today, 10) == 0) selected += bills[i].paidAmount;
        if (choice == 2 && strncmp(bills[i].date, month, 7) == 0) selected += bills[i].paidAmount;
        if (choice == 3 && strncmp(bills[i].date, year, 4) == 0) selected += bills[i].paidAmount;
        if (choice == 4) selected += bills[i].paidAmount;
    }
    if (choice < 1 || choice > 4) { puts(RED "Invalid option." RESET); return; }
    printf(GREEN "Collected income: %.2f Taka\n" RESET, selected);
    printf("Total billed: %.2f Taka\nTotal collected: %.2f Taka\nDue: %.2f Taka\n",
           totalBill, totalPaid, totalBill - totalPaid);
    puts("\n--- Bill Records ---");
    for (i = 0; i < billCount; ++i)
        printf("%s | Patient: %s | Doctor: %s | Bill: %.2f | Paid: %.2f | Due: %.2f\n",
               bills[i].date, bills[i].patientName, bills[i].doctorName,
               bills[i].billAmount, bills[i].paidAmount,
               bills[i].billAmount - bills[i].paidAmount);
}

int adminLogin(void)
{
    char username[50], password[50];
    int attempts;
    printf(MAGENTA "\n====================================================\n");
    printf("                       ADMIN LOGIN\n");
    printf("====================================================\n" RESET);
    for (attempts = 1; attempts <= 3; ++attempts) {
        readText("Username: ", username, sizeof(username));
        readText("Password: ", password, sizeof(password));
        if (!strcmp(username, "master") && !strcmp(password, "1234")) {
            puts(GREEN "\nLogin successful!" RESET); return 1;
        }
        printf(RED "Incorrect username or password! Attempts left: %d\n" RESET,
               3 - attempts);
    }
    puts(RED "Access denied!" RESET);
    return 0;
}

int main(void)
{
    int choice;
    showTitle();
    loadData();
    startLiveClock();
   if (!adminLogin()) {
    stopLiveClock();
    return 0;
}

    do {
#ifdef _WIN32
        system("cls");
#endif
        showTitle();
        showMenu();
        choice = readInt(CYAN "Enter your choice: " RESET);
        switch (choice) {
            case 1: addPatient(); break;
            case 2: displayPatients(); break;
            case 3: sortPatients(); displayPatients(); break;
            case 4: treatPatient(); break;
            case 5: searchPatient(); break;
            case 6: deletePatient(); break;
            case 7: updatePatient(); break;
            case 8: showHistory(); break;
            case 9: saveData(); break;
            case 10: addDoctor(); break;
            case 11: displayDoctors(); break;
            case 12: searchDoctor(); break;
            case 13: updateDoctor(); break;
            case 14: deleteDoctor(); break;
            case 15: showBillingReport(); break;
            case 0:
                saveData(); stopLiveClock();
                puts(GREEN "\nThank you for using HMAP!\nProgram closed successfully." RESET);
                break;
            default: puts(RED "Invalid choice! Please select 0-15." RESET);
        }
        if (choice != 0) pauseScreen();
    } while (choice != 0);
    return 0;
}

