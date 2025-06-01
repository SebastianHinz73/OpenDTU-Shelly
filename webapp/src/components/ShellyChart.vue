<template>
    <div class="row row-cols-1 row-cols-md-3 g-3">
        <div class="col" v-if="liveData.view_option > 0">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('shellyadmin.ShellyPro3em')">
                <h2>
                    {{
                        $n(liveData.cards.pro3em_value, 'decimal', {
                            minimumFractionDigits: liveData.cards.Power.d,
                            maximumFractionDigits: liveData.cards.Power.d,
                        })
                    }}
                    <small class="text-muted">{{ liveData.cards.Power.u }}</small>
                </h2>
                <div class="btn-group" role="group" v-if="liveData.view_option >= 3">
                    {{ liveData.cards.pro3em_debug }}
                </div>
                <div class="col" v-if="liveData.view_option >= 2">
                    <Scatter :data="chartDataPro3em" :options="chartOptions" />
                </div>
            </CardElement>
        </div>

        <div class="col" v-if="liveData.view_option > 0">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('shellyadmin.ShellyPlugS')">
                <h2>
                    {{
                        $n(liveData.cards.plugs_value, 'decimal', {
                            minimumFractionDigits: liveData.cards.Power.d,
                            maximumFractionDigits: liveData.cards.Power.d,
                        })
                    }}
                    <small class="text-muted">{{ liveData.cards.Power.u }}</small>
                </h2>
                <div class="btn-group" role="group" v-if="liveData.view_option >= 3">
                    {{ liveData.cards.plugs_debug }}
                </div>
                <div class="col" v-if="liveData.view_option >= 2">
                    <Scatter :data="chartDataPlugS" :options="chartOptions" />
                </div>
            </CardElement>
        </div>

        <div class="col" v-if="liveData.view_option > 0">
            <CardElement centerContent textVariant="text-bg-success" :text="$t('home.CurrentLimit')">
                <h2>
                    {{
                        $n(liveData.cards.limit_value, 'decimal', {
                            minimumFractionDigits: liveData.cards.Power.d,
                            maximumFractionDigits: liveData.cards.Power.d,
                        })
                    }}
                    <small class="text-muted">{{ liveData.cards.Power.u }}</small>
                </h2>
                <div class="btn-group" role="group" v-if="liveData.view_option >= 3">
                    {{ liveData.cards.limit_debug }}
                </div>
                <div class="col" v-if="liveData.view_option >= 2">
                    <Scatter :data="chartDataLimit" :options="chartOptions" />
                </div>
            </CardElement>
        </div>
    </div>

    <div
        class="row row-cols-1 row-cols-md-3 g-3"
        v-if="
            liveData.view_option >= 2 &&
            (data.data_pro3em.length > 0 || data.data_plugs.length > 0 || data.data_limit.length > 0)
        "
    >
        <Scatter :data="chartDataAll" :options="chartOptions" />
    </div>
</template>

<script lang="ts">
import InterfaceApInfo from '@/components/InterfaceApInfo.vue';
import type { LiveDataGraph, validDataNames, SingleGraph } from '@/types/LiveDataGraph';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';
import CardElement from './CardElement.vue';

import {
    Chart as ChartJS,
    CategoryScale,
    LinearScale,
    PointElement,
    LineElement,
    Title,
    Tooltip,
    Legend,
} from 'chart.js';
import { Scatter } from 'vue-chartjs';

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend);

interface IDatasets {
    label: string;
    fill: boolean;
    borderColor: string;
    backgroundColor: string;
    showLine: boolean;
    borderWidth: number;
    data: DataPoint[];
}

interface DataPoint {
    x: number;
    y: number;
}

let newestXAxis: number = 0;

export default defineComponent({
    components: {
        InterfaceApInfo,
        Scatter,
        CardElement,
    },
    data() {
        return {
            timestamp: 0,
            interval: 0,
            dataLoading: true,
            liveData: {} as LiveDataGraph,
            fetchInterval: 0,
            data: {
                data_pro3em: {} as DataPoint[],
                data_pro3em_min: {} as DataPoint[],
                data_pro3em_max: {} as DataPoint[],
                data_plugs: {} as DataPoint[],
                data_plugs_min: {} as DataPoint[],
                data_plugs_max: {} as DataPoint[],
                data_calculated_limit: {} as DataPoint[],
                data_limit: {} as DataPoint[],
            },

            chartOptions: {
                responsive: true,
                maintainAspectRatio: false,
                elements: {
                    point: {
                        radius: 0,
                    },
                },
                animation: {
                    duration: 0,
                },
                scales: {
                    x: {
                        ticks: {
                            callback: function (value: any) {
                                return value - newestXAxis;
                            },
                        },
                    },
                },
                plugins: {
                    legend: {
                        display: true,
                    },
                },
            },
        };
    },
    created() {
        this.fetchData(true);

        if (this.fetchInterval) {
            clearTimeout(this.fetchInterval);
        }
    },
    unmounted() {
        clearInterval(this.fetchInterval);
    },
    computed: {
        chartDataPro3em: function () {
            return this.graphDataset(this.liveData['diagram_pro3em'], 45);
        },
        chartDataPlugS: function () {
            return this.graphDataset(this.liveData['diagram_plugs'], 45);
        },
        chartDataLimit: function () {
            return this.graphDataset(this.liveData['diagram_limit'], 45);
        },
        chartDataAll: function () {
            return this.graphDataset(this.liveData['diagram_all'], 120);
        },
    },
    methods: {
        fetchData(initialLoading: boolean = false) {
            this.dataLoading = true;
            fetch('/api/livedata/graph?timestamp=' + this.timestamp, { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then((data) => {
                    if (data['timestamp'] === undefined) {
                        this.dataLoading = false;
                        return;
                    }

                    this.liveData = data;
                    this.timestamp = data['timestamp'];
                    this.interval = data['interval'];

                    var interval: number = this.interval;
                    if (initialLoading) {
                        interval = 2000;
                    }

                    this.fetchInterval = setTimeout(() => {
                        this.fetchData();
                    }, interval);

                    if (this.liveData.view_option > 0) {
                        this.handleLoading(initialLoading, 'data_pro3em');
                        this.handleLoading(initialLoading, 'data_pro3em_min');
                        this.handleLoading(initialLoading, 'data_pro3em_max');
                        this.handleLoading(initialLoading, 'data_plugs');
                        this.handleLoading(initialLoading, 'data_plugs_min');
                        this.handleLoading(initialLoading, 'data_plugs_max');
                        this.handleLoading(initialLoading, 'data_calculated_limit');
                        this.handleLoading(initialLoading, 'data_limit');
                    }
                    this.dataLoading = false;
                });
        },
        handleLoading(initialLoading: boolean, name: keyof typeof validDataNames) {
            if (initialLoading) {
                this.data[name] = JSON.parse(this.liveData[name]);
            } else {
                this.data[name] = [...this.data[name], ...JSON.parse(this.liveData[name])];
            }
        },
        graphDataset(graphs: SingleGraph[], length: number) {
            let sets: IDatasets[] = [];
            if (graphs === undefined) {
                return {
                    datasets: [],
                };
            }

            graphs.forEach((element) => {
                if (this.data[element.data_name][this.data[element.data_name].length - 1] === undefined) {
                    return;
                }
                let newestX = this.data[element.data_name][this.data[element.data_name].length - 1].x;
                let max: number = newestX - length;
                newestXAxis = Math.floor(newestX / 10) * 10 + 10;

                let set: IDatasets = {
                    label: element.label,
                    fill: false,
                    borderColor: element.color,
                    backgroundColor: element.color,
                    showLine: true,
                    borderWidth: 2,
                    data: this.data[element.data_name].filter((n) => n.x > max),
                };
                sets.push(set);
            });

            return {
                datasets: sets,
            };
        },
    },
});
</script>
