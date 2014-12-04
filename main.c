/*************************************************************************************
**************************************************************************************
*** The MIT License (MIT)                                                          ***
***                                                                                ***
*** Copyright (c) 2014 Tim Bradt                                                   ***
***`                                                                               ***
*** Permission is hereby granted, free of charge, to any person obtaining a copy   ***
*** of this software and associated documentation files (the "Software"), to deal  ***
*** in the Software without restriction, including without limitation the rights   ***
*** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell      ***
*** copies of the Software, and to permit persons to whom the Software is          ***
*** furnished to do so, subject to the following conditions:                       ***
***                                                                                ***
*** The above copyright notice and this permission notice shall be included in all ***
*** copies or substantial portions of the Software.                                ***
***                                                                                ***
*** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR     ***
*** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,       ***
*** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE    ***
*** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER         ***
*** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  ***
*** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  ***
*** SOFTWARE.                                                                      ***
**************************************************************************************
*************************************************************************************/
#include <stdio.h>
#include <util/vector.h>
#include <util/usefulfunctions.h>
#include <rayTracerCore/ray.h>
#include <rayTracerCore/shapes/geometry.h>
#include <util/colors.h>
#include <util/imageio.h>
#include <rayTracerCore/light.h>
#include <rayTracerCore/material.h>
#include <getopt.h>

#define XRES 512
#define YRES 512
#define XBREAK 0
#define YBREAK 256
#define SAMPLES_X 1
#define SAMPLES_Y 1
#define SCENE 0
#define SHADING 0

const light refLight = {.type = POINT, .ambinentFactor = .1f, .l.point = {.color = {1, 1, 1}, .location = {3, 5, -15}}};
const light sceneLight = {.type = POINT, .ambinentFactor = .1f, .l.point = {.color = {1, 1, 1}, .location = {0, 0.0f, 0}}};
light l;
vector3f lookat = {0, 0, -1};
vector3f lookup = {0, 1, 0};
vector3f camPos = {0, 0, 0};
int buildRef = 0;
int xres = XRES, yres = YRES;
float width = 2, height = 2;
char* file = "scene.png";

void writeImage(char *filename, int width, int height, char *data)
{
    imageio_info info_out;
    info_out.width = width;
    info_out.height = height;
    info_out.depth = 8;
    info_out.quality = 85;
    info_out.colorspace = sRGBColorspace;
    info_out.filename = strdup(filename);
    info_out.comment = NULL;
    info_out.type = CharPixel;
    info_out.map = "RGB";
    imageout(&info_out, data);
    free(info_out.filename);
}

void addToObjectList(object **list, object *newObject)
{
    if (*list == NULL)
    {
        *list = newObject;
    }
    else
    {
        object *tmp = *list;
        while (tmp->next != 0)
        {
            tmp = tmp->next;
        }
        tmp->next = newObject;
    }
}

void trace(const ray r, rayHit *rh, const object *objects)
{
    rh->hit = false;
    rh->mat = EMPTYNESS;
    rayHit testrh;
    testrh.hit = false;
    testrh.depth = rh->depth;
    float time = INFINITY;
    for (const object *obj = objects; obj != NULL; obj = obj->next)
    {
        float hitTime = 0;

        bool hit = obj->hit(r, obj, &hitTime, &testrh);
        if (hit)
            DEBUGOUT(printDepth(rh->depth);
                         printf("Hit: ");
                         printRayHit(testrh));
        if (hit && hitTime < time)
        {
            DEBUGOUT(printDepth(rh->depth);
                         printf("Replacing ");
                         printRayHit(*rh);
                         printf(" with ");
                         printRayHit(testrh));
            time = hitTime;
            *rh = testrh;
        }
    }
    if (rh->hit) DEBUGOUT(printf("\n"));


    if (rh->hit && rh->mat.reflect)
    {
        if (rh->depth > 10)
        {
            return;
        }
        ray reflect;
        vector3f offset = {};
        vector3f_scaleMul_new(offset, rh->normal, rh->offsetError);
        vector3f_add(rh->location, offset);
        reflectRay(&reflect, rh->location, rh->normal, r);
        rh->depth++;
        trace(reflect, rh, objects);
    }
}

void buildScene1(object **list)
{
    DPRINT("Building Scene\n");
    material refl = {.reflect = true, .color = {0, 0, 0}};
    material red = {.reflect = false, .color = {1, 0, 0}};
    material blue = {.reflect = false, .color = {0, 0, 1}};
    material white = {.reflect = false, .color = {1, 1, 1}};

    // Spheres
    vector3f sph1Center = {0, 0, -16};
    float sph1Radius = 2;
    material sph1Mat = refl;
    addToObjectList(list, createSphere(sph1Mat, sph1Radius, sph1Center));
    
    vector3f sph2Center = {3, -1, -14};
    float sph2Radius = 1;
    material sph2Mat = refl;
    addToObjectList(list, createSphere(sph2Mat, sph2Radius, sph2Center));

    vector3f sph3Center = {-3, -1, -14};
    float sph3Radius = 1;
    material sph3Mat = red;
    addToObjectList(list, createSphere(sph3Mat, sph3Radius, sph3Center));

    //back wall
    vector3f back1points[3] = {
        {-8, -2, -20},
        {8,  -2, -20},
        {8,  10, -20}};
    vector3f back2points[3] = {
        {-8, -2, -20},
        {8,  10, -20},
        {-8, 10, -20}};
    material backMat = blue;
    addToObjectList(list, createTriangle(backMat, back1points[0], back1points[1], back1points[2]));
    addToObjectList(list, createTriangle(backMat, back2points[0], back2points[1], back2points[2]));

    //floor
    vector3f floor1points[3] = {
        {-8, -2, -20},
        {8,  -2, -10},
        {8,  -2, -20}};
    vector3f floor2points[3] = {
        {-8, -2, -20},
        {-8, -2, -10},
        {8,  -2, -10}};
    material floorMat = white;
    addToObjectList(list, createTriangle(floorMat, floor1points[0], floor1points[1], floor1points[2]));
    addToObjectList(list, createTriangle(floorMat, floor2points[0], floor2points[1], floor2points[2]));

    // right red triangle
    vector3f rightpoints[3] = {
        {8, -2, -20},
        {8, -2, -10},
        {8, 10, -20}};
    material rightMat = red;
    addToObjectList(list, createTriangle(rightMat, rightpoints[0], rightpoints[1], rightpoints[2]));

}

void buildScene2(object **list)
{
    DPRINT("Building Scene\n");
    material refl = {.reflect = true, .color = {0, 0, 0}};
    material red = {.reflect = false, .color = {1, 0, 0}};
    material blue = {.reflect = false, .color = {0, 0, 1}};
    material white = {.reflect = false, .color = {1, 1, 1}};

    // Spheres
    vector3f sph1Center = {0, 0, 0};
    float sph1Radius = 0;
    material sph1Mat = blue;

    time_t t;
    /* Intializes random number generator */
    srand((unsigned) time(&t));
    for(int i = 0; i < 1000; i++)
    {
        float locationR = mapToRangef(((float)rand()/(float)(RAND_MAX)), 0, 1, 4, 18);
        float phi = (float) (mapToRangef(((float)rand()/(float)(RAND_MAX)), 0, 1, -180, 180) * M_PI / 180);
        float rnd = mapToRangef(((float)rand()/(float)(RAND_MAX)), 0, 1, .4, .96);
        float theta = acosf(2*rnd - 1);
        sph1Center[0] = locationR * sinf(theta) * cosf(phi);
        sph1Center[1] = locationR * cosf(theta);
        sph1Center[2] = -locationR * sinf(theta) * sinf(phi);
        sph1Radius = mapToRangef(((float)rand()/(float)(RAND_MAX)), 0, 1, .1, .5);
        sph1Mat.reflect = (bool) (rand() & 2);
        for(int x = 0; x < 3; x++) sph1Mat.color[x] = ((float)rand()/(float)(RAND_MAX));
        addToObjectList(list, createSphere(sph1Mat, sph1Radius, sph1Center));
    }

    //floor
    vector3f floor1points[3] = {
        {-8, -10+8, 10*2},
        {8,  -10+8, 10*2},
        {8,  -10+8, -10*2}};
    vector3f floor2points[3] = {
        {-8, -10+8,  10*2},
        { 8, -10+8, -10*2},
        {-8, -10+8, -10*2}};
    material floorMat = white;
    addToObjectList(list, createTriangle(floorMat, floor1points[0], floor1points[1], floor1points[2]));
    addToObjectList(list, createTriangle(floorMat, floor2points[0], floor2points[1], floor2points[2]));
}

void parseArguments(int argc, char **argv)
{
    while (1)
    {
        static struct option long_options[] =
            {
                /* These options set a flag. */
                {"ref",     no_argument,  &buildRef, 1},
                /* These options don’t set a flag.
                   We distinguish them by their indices. */
                {"xres",    required_argument,  0, 'x'},
                {"yres",    required_argument,  0, 'y'},
                {"width",   required_argument,  0, 'w'},
                {"height",  required_argument,  0, 'h'},
                {"file",    required_argument,  0, 'f'},
                {0, 0, 0, 0}
            };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        int c = getopt_long (argc, argv, "x:y:f:",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                    printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 'x':
                printf ("option -x with value `%s'\n", optarg);
                xres = atoi(optarg);
                break;

            case 'y':
                printf ("option -x with value `%s'\n", optarg);
                yres = atoi(optarg);
                break;

            case 'w':
                printf ("option -c with value `%s'\n", optarg);
                width = (float) atof(optarg);
                break;

            case 'h':
                printf ("option -d with value `%s'\n", optarg);
                height = (float) atof(optarg);
                break;

            case 'f':
                printf ("option -f with value `%s'\n", optarg);
                file = optarg;
                break;

            case '?':
                /* getopt_long already printed an error message. */
                exit(1);
                break;

            default:
                abort ();
        }
    }
}

int main(int argc, char **argv)
{
    parseArguments(argc, argv);
    char *image = calloc((size_t) (xres * yres * 3), sizeof(char));



    camera cam;
    setCamera(&cam, camPos, lookat, lookup);
    perspective p = {.cam = cam, .height = height, .width = width, .res_x = (unsigned int) xres, .res_y = (unsigned int) yres, .viewPlaneDistance = 2};
    object *objects = NULL;

    do
    {
        //Build List
        if (buildRef)
        {
            l = refLight;
            file = "reference.png";
            buildScene1(&objects);
        }
        else
        {
            l = sceneLight;
            buildScene2(&objects);
        }
        buildRef++;


        for (unsigned int y = 0; y < p.res_y; y++)
        {
            for (unsigned int x = 0; x < p.res_x; x++)
            {
                if (x == XBREAK && y == YBREAK)
                {
                    volatile int b = 1;
                    b++;
                }
                sampler samples;
                getSampler(&samples, SAMPLES_X, SAMPLES_Y, p, x, y, GLOBAL);
                rayHit samplesRayHits[samples.numOfSamplesX * samples.numOfSamplesY];
                for (unsigned int sample = 0; sample < samples.numOfSamplesX * samples.numOfSamplesY; sample++)
                {
                    samplesRayHits[sample].depth = 0;
                    trace(samples.rays[sample], &samplesRayHits[sample], objects);
                    samplesRayHits[sample].objects = objects;
                    if (samplesRayHits[sample].hit)
                    {
                        float ambinentFactor = l.ambinentFactor;
                        float diffuse = getDiffuseFactor(samplesRayHits[sample], l);
                        float specular = getSpecularFactor(samplesRayHits[sample], l);
                        if (inShadow(samplesRayHits[sample], l))
                        {
                            DEBUGOUT(printf("In Shadow"));
                            diffuse = 0.0f;
                            specular = 0.0f;
                        }
                        DEBUGOUT(printf("Ambinent: %4.4f Diffuse: %4.4f Specular: %4.4f\n", ambinentFactor, diffuse, specular));
                        vector3f ambientColor = {}, diffuseColor = {}, specularColor = {};
                        vector3f_scaleMul_new(ambientColor, samplesRayHits[sample].mat.color, ambinentFactor);
                        vector3f_scaleMul_new(diffuseColor, samplesRayHits[sample].mat.color, diffuse);
                        vector3f_scaleMul_new(specularColor, samplesRayHits[sample].mat.color, specular);
                        vector3f_copy(samplesRayHits[sample].mat.color, ambientColor);
                        vector3f_add(samplesRayHits[sample].mat.color, diffuseColor);
                        vector3f_add(samplesRayHits[sample].mat.color, specularColor);
                    }
                }
                vector3f color = {};
                getFinalColor(color, samplesRayHits, samples.numOfSamplesX * samples.numOfSamplesY);
#define MIN(x, y) ((x < y) ? x : y)
                image[XY2INDEX(x, y, 0, XRES, 3)] = (char) mapToRangef(MIN(color[0], 1), 0, 1, 0, 255);
                image[XY2INDEX(x, y, 1, XRES, 3)] = (char) mapToRangef(MIN(color[1], 1), 0, 1, 0, 255);
                image[XY2INDEX(x, y, 2, XRES, 3)] = (char) mapToRangef(MIN(color[2], 1), 0, 1, 0, 255);
#undef MIN
                cleanSampler(&samples);
            }
        }
        cleanObjectList(&objects);
        writeImage(file, xres, yres, image);
    } while(buildRef < 2);
    free(image);
    return 0;
}