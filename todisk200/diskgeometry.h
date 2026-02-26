#ifndef DISKGEOMETRY_H
#define DISKGEOMETRY_H


#define COUNTOF_TRACK               80
#define COUNTOF_SECTOR_PER_TRACK    16

#define SIZEOF_SECTOR               256
#define SIZEOF_TRACK                (COUNTOF_SECTOR_PER_TRACK*SIZEOF_SECTOR)
#define SIZEOF_SIDE                 (COUNTOF_TRACK*SIZEOF_TRACK)


#endif  /* DISKGEOMETRY_H */
