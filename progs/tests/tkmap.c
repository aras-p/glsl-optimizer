
enum {
    COLOR_BLACK = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE
};

static float RGBMap[9][3] = {
    {0, 0, 0},
    {1, 0, 0},
    {0, 1, 0},
    {1, 1, 0},
    {0, 0, 1},
    {1, 0, 1},
    {0, 1, 1},
    {1, 1, 1},
    {0.5, 0.5, 0.5}
};

static void SetColor(int c)
{
    if (glutGet(GLUT_WINDOW_RGBA))
        glColor3fv(RGBMap[c]);
    else
        glIndexf(c);
}

static void InitMap(void)
{
    int i;

    if (rgb)
	return;

    for (i = 0; i < 9; i++)
	    glutSetColor(i, RGBMap[i][0], RGBMap[i][1], RGBMap[i][2]);
}

static void SetFogRamp(int density, int startIndex)
{
    int fogValues, colorValues;
    int i, j, k;
    float intensity;

    fogValues = 1 << density;
    colorValues = 1 << startIndex;
    for (i = 0; i < colorValues; i++) {
	for (j = 0; j < fogValues; j++) {
	    k = i * fogValues + j;
	    intensity = (i * fogValues + j * colorValues) / 255.0;
	    glutSetColor(k, intensity, intensity, intensity);
	}
    }
}

static void SetGreyRamp(void)
{
    int i;
    float intensity;

    for (i = 0; i < 255; i++) {
	intensity = i / 255.0;
	glutSetColor(i, intensity, intensity, intensity);
    }
}

