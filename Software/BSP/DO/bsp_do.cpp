#include "main.h"
#include "bsp_do.h"

extern "C"
{
#include "dwt.h"
}

void MX_GPIO_Init(void);
void MX_TIM1_Init(void);
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *timHandle);
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *tim_baseHandle);

TIM_HandleTypeDef htim1;

void BSP::DO::init()
{
    MX_GPIO_Init();
    MX_TIM1_Init();
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    set_signal(0, 50);

    return;
}

void BSP::DO::set_signal(uint16_t freq, uint8_t duty)
{
    uint32_t counter = 0;
    tim_cfg_t cfg = {};

    DWT_CYCCNT = 0U;
    cfg = tim_pick(72000000U, freq, 16U, static_cast<double>(duty) / 100.0);
    counter = DWT_CYCCNT;

    TIM1->ARR = cfg.arr;
    TIM1->PSC = cfg.psc;
    if (duty == 100)
    {
        TIM1->CCR1 = 0xFFFFU;
    }
    else
    {
        TIM1->CCR1 = cfg.ccr;
    }

    return;
}

uint32_t BSP::DO::u32_abs_diff(uint32_t a, uint32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

BSP::DO::tim_cfg_t BSP::DO::tim_pick(uint32_t f_tim, uint32_t f_hz, uint8_t arr_bits, double duty)
{
    tim_cfg_t best = {};
    uint32_t arr_max = (arr_bits == 32) ? 0xFFFFFFFFu : 0xFFFFu;

    // защита от деления на 0
    if (f_hz == 0 || f_tim == 0)
    {
        best.psc = 0;
        best.arr = 0;
        best.ccr = 0;
        best.f_real = 0;
        return best;
    }

    uint32_t best_err = 0xFFFFFFFFu;

    for (uint32_t psc = 0; psc <= 0xFFFFu; psc++)
    {
        // вычисляем ARR примерно как round(f_tim/((psc+1)*f_hz) - 1)
        // используем целочисленное округление:
        uint64_t denom = static_cast<uint64_t>(psc + 1) * static_cast<uint64_t>(f_hz);
        uint64_t q = static_cast<uint64_t>(f_tim);

        if (denom == 0)
            continue;

        // значение (ARR+1) ≈ round(f_tim/denom)
        uint64_t arrp1 = static_cast<uint64_t>((static_cast<double>(q) + static_cast<double>(denom) / 2.0) / static_cast<double>(denom)); // round()

        if (arrp1 == 0)
            continue; // ARR был бы -1
        if (arrp1 > static_cast<uint64_t>(arr_max) + 1u)
            continue;

        uint32_t arr = static_cast<uint32_t>(arrp1 - 1u);

        // фактическая частота
        uint32_t f_real = static_cast<uint32_t>(static_cast<double>(f_tim) / (static_cast<double>(psc + 1u) * static_cast<double>(arr + 1u)));

        uint32_t err = u32_abs_diff(f_real, f_hz);
        if (err < best_err)
        {
            best_err = err;
            best.psc = static_cast<uint16_t>(psc);
            best.arr = arr;
            best.f_real = f_real;

            // CCR по duty (0..1)
            if (duty < 0.0f)
                duty = 0.0f;
            if (duty > 1.0f)
                duty = 1.0f;

            uint32_t ccr = static_cast<uint32_t>((duty * static_cast<double>(arr + 1u)) + 0.5f); // round
            if (ccr > arr)
                ccr = arr;
            best.ccr = ccr;

            if (best_err == 0)
                break; // идеальное попадание
        }
    }

    return best;
}

void BSP::DO::enable_pull_up()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    return;
}

void BSP::DO::disable_pull_up()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    return;
}

void BSP::DO::enable_nv()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
    return;
}

void BSP::DO::disable_nv()
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
    return;
}

void MX_GPIO_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);

    /*Configure GPIO pins : PB0 PB1 */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* TIM1 init function */
void MX_TIM1_Init(void)
{

    /* USER CODE BEGIN TIM1_Init 0 */

    /* USER CODE END TIM1_Init 0 */

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    /* USER CODE BEGIN TIM1_Init 1 */

    /* USER CODE END TIM1_Init 1 */
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 0;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 65535;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter = 0;
    sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
    sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
    sBreakDeadTimeConfig.Break2Filter = 0;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN TIM1_Init 2 */

    /* USER CODE END TIM1_Init 2 */
    HAL_TIM_MspPostInit(&htim1);
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *tim_baseHandle)
{

    if (tim_baseHandle->Instance == TIM1)
    {
        /* USER CODE BEGIN TIM1_MspInit 0 */

        /* USER CODE END TIM1_MspInit 0 */
        /* TIM1 clock enable */
        __HAL_RCC_TIM1_CLK_ENABLE();
        /* USER CODE BEGIN TIM1_MspInit 1 */

        /* USER CODE END TIM1_MspInit 1 */
    }
}
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *timHandle)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (timHandle->Instance == TIM1)
    {
        /* USER CODE BEGIN TIM1_MspPostInit 0 */

        /* USER CODE END TIM1_MspPostInit 0 */

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**TIM1 GPIO Configuration
        PA8     ------> TIM1_CH1
        */
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF6_TIM1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USER CODE BEGIN TIM1_MspPostInit 1 */

        /* USER CODE END TIM1_MspPostInit 1 */
    }
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *tim_baseHandle)
{

    if (tim_baseHandle->Instance == TIM1)
    {
        /* USER CODE BEGIN TIM1_MspDeInit 0 */

        /* USER CODE END TIM1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_TIM1_CLK_DISABLE();
        /* USER CODE BEGIN TIM1_MspDeInit 1 */

        /* USER CODE END TIM1_MspDeInit 1 */
    }
}