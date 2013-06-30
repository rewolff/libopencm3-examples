/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 * Copyright (C) 2011 Stephen Caudle <scaudle@doceme.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <stdarg.h>


static void clock_setup(void)
{
	/* Enable GPIOD clock for LED & USARTs. */
	rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);
	rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPAEN);

	/* Enable clocks for USART2. */
	rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_USART2EN);
}

static void usart_setup(void)
{
	/* Setup USART2 parameters. */

  // XXX TODO: figure out why the number passed is /4 for STM32F3. 
	usart_set_baudrate(USART2, 115200/4);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX | USART_MODE_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	USART_ICR (USART2) = USART_ICR_ORECF | USART_ICR_FECF;

	/* Finally enable the USART. */
	usart_enable(USART2);
}

static void gpio_setup(void)
{
	/* Setup GPIO pin GPIO12 on GPIO port D for LED. */
	gpio_mode_setup(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);

	/* Setup GPIO pins for USART2 transmit. */
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2|GPIO3);

	/* Setup USART2 RX/TX pins as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2 | GPIO3);
}


int main(void)
{
	int i, j = 0, c = 0;

	clock_setup();
	gpio_setup();
	usart_setup();

	/* Blink the LED (PE12) on the board with every transmitted byte. */
	while (1) {
		gpio_toggle(GPIOE, GPIO12);	/* LED on/off */
		usart_send_blocking(USART2, c + '0'); /* USART2: Send byte. */
		c = (c == 9) ? 0 : c + 1;	/* Increment c. */

		// The original finished the line at 80 chars. 39 gives
		// a nice twirl effect and allows for reception characters
		// to be printed inbetween. 
		if ((j++ % 39) == 0) {		/* Newline after line full. */
			usart_send_blocking(USART2, '\r');
			usart_send_blocking(USART2, '\n');
		}
		for (i = 0; i < 300000; i++)	/* Wait a bit. */
			__asm__("NOP");

		if ((USART_ISR(USART2) & USART_ISR_RXNE) != 0) {
		  usart_send_blocking (USART2, '<');
		  usart_send_blocking (USART2, USART_RDR(USART2));
		  usart_send_blocking (USART2, '>');
		}
		if ((USART_ISR(USART2) & USART_ISR_ORE) != 0) {
		  USART_ICR (USART2) = USART_ICR_ORECF;
		  usart_send_blocking (USART2, 'R');
		}
		if ((USART_ISR(USART2) & USART_ISR_PE) != 0) {
		  USART_ICR (USART2) = USART_ICR_PECF;
		  usart_send_blocking (USART2, 'P');
		}
		if ((USART_ISR(USART2) & USART_ISR_FE) != 0) {
		  USART_ICR (USART2) = USART_ICR_FECF;
		  usart_send_blocking (USART2, 'F');
		}
	}

	return 0;
}
