
typedef struct {
    int sizeX, sizeY;
    GLubyte *data;
} PPMImage;

static PPMImage *LoadPPM(const char *filename)
{
    char buff[16];
    PPMImage *result;
    FILE *fp;
    int maxval;

    fp = fopen(filename, "rb");
    if (!fp)
    {
	fprintf(stderr, "Unable to open file `%s'\n", filename);
	exit(1);
    }

    if (!fgets(buff, sizeof(buff), fp))
    {
	perror(filename);
	exit(1);
    }

    if (buff[0] != 'P' || buff[1] != '6')
    {
	fprintf(stderr, "Invalid image format (must be `P6')\n");
	exit(1);
    }

    result = malloc(sizeof(PPMImage));
    if (!result)
    {
	fprintf(stderr, "Unable to allocate memory\n");
	exit(1);
    }

    if (fscanf(fp, "%d %d", &result->sizeX, &result->sizeY) != 2)
    {
	fprintf(stderr, "Error loading image `%s'\n", filename);
	exit(1);
    }

    if (fscanf(fp, "%d", &maxval) != 1)
    {
	fprintf(stderr, "Error loading image `%s'\n", filename);
	exit(1);
    }

    while (fgetc(fp) != '\n')
	;

    result->data = malloc(3 * result->sizeX * result->sizeY);
    if (!result)
    {
	fprintf(stderr, "Unable to allocate memory\n");
	exit(1);
    }

    if (fread(result->data, 3 * result->sizeX, result->sizeY, fp) != result->sizeY)
    {
	fprintf(stderr, "Error loading image `%s'\n", filename);
	exit(1);
    }

    fclose(fp);

    return result;
}

