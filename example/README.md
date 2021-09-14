So this an example that can be used with any STM32F4xx microcontroller.

Note that any timer can be used, I used TIM8 because then we can go all the way to Dshot 2400
as TIM8 and TIM1 are on the APB2 bus so recive that goood 168Mhz clock
(The max I was able to get with the timers on the APB1 bus was Dshot1200, although we could go to 2400 with tuning
maybe lol)

Also keep in mind we can use the same timer's other channels for more motors too, I havent showed this yet but
please feel free to give it a shot!