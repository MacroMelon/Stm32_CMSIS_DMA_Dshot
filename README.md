# Stm32_CMSIS_DMA_Dshot
A low level, VERY lightweight dshot (upto Dshot2400) implementation on stm32 using the standard device headers (CMSIS) proved by ST

For now, everything you need should be in the examples folder, keep in mind this is just something that I came up with
for a personal project, so its not a proper library or anything

So by default it runs on Dshot1200, we will have to set the ARR to 14 and prescaler accordingly to go to 2400

Inspired and made possible by: https://github.com/mokhwasomssi/stm32_hal_dshot

TODO- proper docs

###This sofware is just pulled from one of my projects, check before using, may not always work first try###

###This is VERY early in the dev cycle so be careful when using it, I havent done any testing so I take no responsisblity, legal or otherwise, for damages caused by others due to bugs in this code###
