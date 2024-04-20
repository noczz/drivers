#ifndef _GPIO_H_
#define _GPIO_H_

#define BUSADR_PERIPHERALS	0x7E000000LU
#define BUSADR_GPIO_BASE	0x7E200000LU

#define PERIPHERALS_BASE	0x3F000000LU
#define GPIO_SHIFT		BUSADR_GPIO_BASE - BUSADR_PERIPHERALS

#define GPIO_BASE		PERIPHERALS_BASE + GPIO_SHIFT
#define GPIO_WIDTH 0xB4

#define GPIO_FSEL0_SHIFT	0x00
#define GPIO_SET0_SHIFT		0x1C
#define GPIO_CLR0_SHIFT		0x28

#define FSEL_WIDTH 3
#define MASK_FSEL(n) (7 << n*FSEL_WIDTH)
#define SET_GPFSEL(n, val) (val << n*FSEL_WIDTH)

#define GPIN  0
#define GPOUT 1

#define SET_WIDTH 1
#define MASK_SET(n) (1 << n*SET_WIDTH)
#define SET_GPSET(n, val) (val << n*SET_WIDTH)

#define CLR_WIDTH 1
#define MASK_CLR(n) (1 << n*CLR_WIDTH)
#define SET_GPCLR(n, val) (val << n*SET_WIDTH)

#endif
