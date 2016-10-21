// nolint
//  Sin_Table.h
//  SquareCam
//
//  Created by  on 12-5-5.
//  Copyright (c) 2012年 __MyCompanyName__. All rights reserved.
//

#ifndef SquareCam_Sin_Table_h
#define SquareCam_Sin_Table_h


extern double sin_tables[1025];

// x: [0~4096]
#define GET_SIN(x, yOut)           \
if (x <= 1024) {                \
    yOut = sin_tables[x];          \
} else if (x <= 2048) {         \
    yOut = sin_tables[2048 - x];   \
} else if (x <= 3072) {         \
    yOut = -sin_tables[x - 2048];  \
} else {                        \
    yOut = -sin_tables[4096 - x];  \
}

#define GET_COS(x, yOut)           \
if (x <= 1024) {                \
    yOut = sin_tables[1024 - x];   \
} else if (x <= 2048) {         \
    yOut = -sin_tables[x - 1024];  \
} else if (x <= 3072) {         \
    yOut = -sin_tables[3072 - x];  \
} else {                        \
    yOut = sin_tables[x - 3072];   \
}

#endif
