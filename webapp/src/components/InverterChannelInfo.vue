<template>
    <div
        class="card card-table"
        :class="{
            'border-info': channelType == 'AC',
            'border-secondary': channelType == 'INV',
        }"
    >
        <div v-if="channelType == 'INV'" class="card-header text-bg-secondary">
            {{ $t('inverterchannelinfo.General') }}
        </div>

        <div v-if="channelType == 'DC'" class="card-header">
            <template v-if="channelData.name.u != ''">{{ channelData.name.u }}</template>
            <template v-else>{{ $t('inverterchannelinfo.String', { num: channelNumber + 1 }) }}</template>
        </div>

        <div v-if="channelType == 'AC'" class="card-header text-bg-info">
            {{ $t('inverterchannelinfo.Phase', { num: channelNumber + 1 }) }}
        </div>

        <div class="table-responsive">
            <table class="table table-striped table-hover">
                <tbody>
                    <tr v-for="(property, key) in channelData" :key="`prop-${key}`">
                        <template v-if="key != 'name' && property">
                            <th scope="row">{{ $t('inverterchannelproperty.' + key) }}</th>
                            <td class="value">
                                {{
                                    $n(property.v, 'decimal', {
                                        minimumFractionDigits: property.d,
                                        maximumFractionDigits: property.d,
                                    })
                                }}
                            </td>
                            <td>{{ property.u }}</td>
                        </template>
                    </tr>
                </tbody>
            </table>
        </div>
    </div>
</template>

<script lang="ts">
import type { InverterStatistics } from '@/types/LiveDataStatus';
import { defineComponent, type PropType } from 'vue';

export default defineComponent({
    props: {
        channelData: { type: Object as PropType<InverterStatistics>, required: true },
        channelType: { type: String, required: true },
        channelNumber: { type: Number, required: true },
    },
});
</script>
