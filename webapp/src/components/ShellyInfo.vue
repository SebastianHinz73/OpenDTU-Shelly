<template>
    <div class="row row-cols-1 row-cols-md-3 g-3">
        <div class="col">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.TotalYieldDay')">
                <h2>
                    {{ $n(shellyData.pro3em_value, 'decimal') }}
                </h2>
            </CardElement>
            <CardElement centerContent textVariant="text-bg-success" :text="$t('invertertotalinfo.TotalYieldDay')">
                <h2>
                    {{ $n(shellyData.plugs_value, 'decimal') }}
                </h2>
            </CardElement>
        </div>
    </div>
</template>

<script lang="ts">
import type { Shelly } from '@/types/LiveDataStatus';
import CardElement from './CardElement.vue';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    components: {
        CardElement,
    },
    props: {
        shellyData: { type: Object as PropType<Shelly>, required: true },
    },
});
</script>
