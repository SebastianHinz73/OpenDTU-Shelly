<template>
    <div class="row row-cols-1 row-cols-md-3 g-3">
        <div class="col" v-if="shellyData.pro3em_enabled">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('shellyadmin.ShellyPro3em')">
                <h2>
                    {{
                        $n(shellyData.pro3em_value, 'decimal', {
                            minimumFractionDigits: totalData.Power.d,
                            maximumFractionDigits: totalData.Power.d,
                        })
                    }}
                    <small class="text-muted">{{ totalData.Power.u }}</small>
                </h2>
                <div class="btn-group" role="group">
                    {{ shellyData.pro3em_debug }}
                </div>
            </CardElement>
        </div>
        <div class="col" v-if="shellyData.plugs_enabled">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('shellyadmin.ShellyPlugS')">
                <h2>
                    {{
                        $n(shellyData.plugs_value, 'decimal', {
                            minimumFractionDigits: totalData.Power.d,
                            maximumFractionDigits: totalData.Power.d,
                        })
                    }}
                   <small class="text-muted">{{ totalData.Power.u }}</small>
                </h2>
                <div class="btn-group" role="group">
                    {{ shellyData.plugs_debug }}
                </div>
            </CardElement>
        </div>
        <div class="col" v-if="shellyData.limit_enabled">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('home.CurrentLimit')">
                <h2>
                    {{
                        $n(shellyData.limit_value, 'decimal', {
                            minimumFractionDigits: totalData.Power.d,
                            maximumFractionDigits: totalData.Power.d,
                        })
                    }}
                   <small class="text-muted">{{ totalData.Power.u }}</small>
                </h2>
                <div class="btn-group" role="group">
                    {{ shellyData.debug }}
                </div>
            </CardElement>
        </div>
    </div>
</template>

<script lang="ts">
import type { Shelly, Total } from '@/types/LiveDataStatus';
import CardElement from './CardElement.vue';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        shellyData: { type: Object as PropType<Shelly>, required: true },
        totalData: { type: Object as PropType<Total>, required: true },
    },
    methods: {
        buttonClick() {
            //this.consoleBuffer = '';
        },

    },
});
</script>
