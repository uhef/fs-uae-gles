
extern void gayle_reset (int);
extern void gayle_hsync (void);
extern int gayle_add_ide_unit (int ch, const TCHAR *path, int blocksize, int readonly,
		       const TCHAR *devname, int cyls, int sectors, int surfaces, int reserved,
		       int bootpri, const TCHAR *filesys, int pcyls, int pheads, int psecs);
extern int gayle_modify_pcmcia_sram_unit (const TCHAR *path, int readonly, int insert);
extern int gayle_modify_pcmcia_ide_unit (const TCHAR *path, int readonly, int insert);
extern int gayle_add_pcmcia_sram_unit (const TCHAR *path, int readonly);
extern int gayle_add_pcmcia_ide_unit (const TCHAR *path, int readonly);
extern void gayle_free_units (void);
extern void rethink_gayle (void);
extern void gayle_map_pcmcia (void);

#define PCMCIA_COMMON_START 0x600000
#define PCMCIA_COMMON_SIZE 0x400000
