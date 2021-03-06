#include <minix/syslib.h>
#include <minix/drivers.h>
#include <machine/int86.h>
#include "vbe.h"
#include "lmlib.h"
#include "i8042.h"

#define LINEAR_MODEL_BIT 14

#define PB2BASE(x) (((x) >> 4) & 0x0F000)
#define PB2OFF(x) ((x) & 0x0FFFF)

int vbe_get_mode_info(unsigned short mode, vbe_mode_info_t *vmi_p)
{
	mmap_t video_map;
	struct reg86u reg86;

	if (lm_init() == NULL)
		return 1;

	if (lm_alloc(sizeof(vbe_mode_info_t), &video_map) == NULL)
	{
		printf("Error on lm_alloc() - get_mode_info()\n");
		return 1;
	}

	reg86.u.b.ah = VBE_AH;
	reg86.u.b.al = VBE_AL;

	reg86.u.b.intno = VBE_INT10;

	reg86.u.w.cx = mode;
	reg86.u.w.es = PB2BASE(video_map.phys);
	reg86.u.w.di = PB2OFF(video_map.phys);

	if (sys_int86(&reg86) != OK)
	{
		if ((reg86.u.b.ah == 0x03) || (reg86.u.b.ah == 0x02) || (reg86.u.b.ah == 0x01))
		{
			lm_free(&video_map);
			printf("Error on sys_int86() call - get_mode_info()\n");
			return 1;
		}
	} else if (reg86.u.b.ah == 0x00 && reg86.u.b.al == 0x4F)
	{
		*vmi_p = *((vbe_mode_info_t*) video_map.virtual);
		lm_free(&video_map);
	}
	return 0;
}

uint16_t* vbe_get_information(vbe_information_block_t *info_ctrl)
{
	struct reg86u reg86;
	mmap_t video_map;


	if (lm_init() == NULL)
	{
		printf("Error on lm_init() - NULL return\n");
		return NULL;
	}


	if (lm_alloc(sizeof(vbe_information_block_t), &video_map) == NULL)
	{
		printf("Error on lm_alloc() - vbe_get_information()\n");
		return NULL;
	}

	//VBE MODE 2
	info_ctrl->vbe_signature[0] = 'V';
	info_ctrl->vbe_signature[1] = 'E';
	info_ctrl->vbe_signature[2] = 'S';
	info_ctrl->vbe_signature[3] = '2';


	reg86.u.w.es = PB2BASE(video_map.phys);
	reg86.u.w.di = PB2OFF(video_map.phys);
	reg86.u.b.ah = VBE_AH;
	reg86.u.b.al = VBE_SET_VIDEO_MODE;
	reg86.u.b.intno = VBE_INT10;

	if (sys_int86(&reg86) != OK)
	{
		//TEST FAIL CALLS
		if ((reg86.u.b.ah == 0x03) || (reg86.u.b.ah == 0x02) || (reg86.u.b.ah == 0x01))
		{
			lm_free(&video_map);
			printf("Error on sys_int86() call - get_mode_info()\n");
			return NULL;
		}
	} else if (reg86.u.b.al == 0x4F && reg86.u.b.ah == 0x00)
	{
		//POINTING TO VIRTUAL ADDRESS
		*info_ctrl = *((vbe_information_block_t*) video_map.virtual);

		void* ptr = (void *) ((info_ctrl->video_mode_ptr & 0xFFFF0000) >> 12);
		ptr += PB2OFF(info_ctrl->video_mode_ptr);
		ptr += (uint32_t) video_map.virtual & 0xF0000000;

		//CLEAR MAPPED MEMORY
		lm_free(&video_map);
		return ptr;
	}

}

