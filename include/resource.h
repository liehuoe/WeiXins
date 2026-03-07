#define IDI_LOGO 101
#define IDD_LOGO 102
#ifdef _DEBUG
    #define PAGE(name) "http://localhost:5173?page="#name
#else
    #define PAGE(name) "http://./1000.html?page="#name
#endif
