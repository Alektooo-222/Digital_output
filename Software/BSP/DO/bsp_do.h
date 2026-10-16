#ifndef BSP_DO_H
#define BSP_DO_H

namespace BSP
{
    class DO
    {
    public:
        static void init();
        /* static void set_freq(uint16_t fr); */
        static uint16_t get_freq();

        static void set_signal(uint16_t freq, uint8_t duty);
        static uint16_t get_duty();

        static void enable_pull_up();
        static void disable_pull_up();
        static void enable_nv(); // nv - negative voltage
        static void disable_nv();

    private:
        typedef struct
        {
            uint32_t psc;
            uint32_t arr;
            uint32_t ccr;    // если PWM не нужен — можно игнорировать
            uint32_t f_real; // фактическая частота, Гц
        } tim_cfg_t;

        static uint32_t u32_abs_diff(uint32_t a, uint32_t b);
        static inline tim_cfg_t tim_pick(uint32_t f_tim, uint32_t f_hz, uint8_t arr_bits, double duty);
    };
}
#endif