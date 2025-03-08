<template>
  <div class="row row-cols-1 row-cols-md-3 g-3">
    <div class="col">
      <Scatter :data="chartDataPro3em" :options="chartOptions" />
    </div>
    <div class="col">
      <Scatter :data="chartDataPlugS" :options="chartOptions" />
    </div>
    <div class="col">
      <Scatter :data="chartDataLimit" :options="chartOptions" />
    </div>
  </div>

  <div class="row row-cols-1 row-cols-md-3 g-3">
    <Scatter :data="chartDataAll" :options="chartOptions" />
  </div>
</template>

<script lang="ts">
import InterfaceApInfo from '@/components/InterfaceApInfo.vue';
import type { LiveDataGraph, validDataNames, SingleGraph } from '@/types/LiveDataGraph';
import { authHeader, handleResponse } from '@/utils/authentication';
import { defineComponent } from 'vue';

import {
  Chart as ChartJS,
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
} from 'chart.js'
import { Scatter } from 'vue-chartjs'

ChartJS.register(
  CategoryScale,
  LinearScale,
  PointElement,
  LineElement,
  Title,
  Tooltip,
  Legend
)

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

export default defineComponent({
  components: {
      InterfaceApInfo,
      Scatter,
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
              point:{
                  radius: 0
              }
          },
          animation: {
            duration: 0
          },
          /*
          scales: {
                    xAxes: [{
                        ticks: {
                            min: 2000,
                            beginAtZero: true
                        }
                    }]
                },*/
          plugins: {
            legend: {
              display: true
            } 
          }
        }
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
    chartDataPro3em: function() {
      return this.graphDataset(this.liveData["diagram_pro3em"], 45);
    },
    chartDataPlugS: function() {
      return this.graphDataset(this.liveData["diagram_plugs"], 45);
    },
    chartDataLimit: function() {
      return this.graphDataset(this.liveData["diagram_limit"], 45);
    },
    chartDataAll: function() {
      return this.graphDataset(this.liveData["diagram_all"], 120);
    },
  },
  methods: {
    fetchData(initialLoading: boolean = false) {
        this.dataLoading = true;
        fetch('/api/livedata/graph?timestamp=' + this.timestamp, { headers: authHeader() })
            .then((response) => handleResponse(response, this.$emitter, this.$router))
            .then((data) => {
                this.liveData = data;
                this.timestamp = data["timestamp"];
                this.interval = data["interval"]; 
             
                var interval:number = this.interval;
                if(initialLoading)
                {
                  interval = 2000;
                }

                this.fetchInterval = setTimeout(() => {
                  //console.log("timer");
                  this.fetchData();
                }, interval);

                this.handleLoading(initialLoading, "data_pro3em");
                this.handleLoading(initialLoading, "data_pro3em_min");
                this.handleLoading(initialLoading, "data_pro3em_max");
                this.handleLoading(initialLoading, "data_plugs");
                this.handleLoading(initialLoading, "data_plugs_min");
                this.handleLoading(initialLoading, "data_plugs_max");
                this.handleLoading(initialLoading, "data_calculated_limit");
                this.handleLoading(initialLoading, "data_limit");

                this.dataLoading = false;
            });
    },
    handleLoading(initialLoading: boolean, name: keyof typeof validDataNames) {
      if(initialLoading)
      {
        this.data[name] = JSON.parse(this.liveData[name]);
      }
      else
      {
        this.data[name] = [...this.data[name], ...JSON.parse(this.liveData[name])];
      }
    },
    graphDataset(graphs: SingleGraph[], length: number) {
   
      let sets : IDatasets[] = [];
      if(graphs === undefined) 
      {
        return {
          datasets: [
          ]
        };
      }

      graphs.forEach((element) => {
        if(this.data[element.data_name][this.data[element.data_name].length-1] === undefined)
        {
          return;
        }
        let max: number = this.data[element.data_name][this.data[element.data_name].length-1].x - length;

        let set: IDatasets = {
          label: element.label,
          fill: false,
          borderColor: element.color,
          backgroundColor: element.color,
          showLine: true,
          borderWidth: 2,
          data: this.data[element.data_name].filter(n => n.x > max),
        };
        sets.push(set);
      });

      return {
        datasets: sets
      };
    },
  },

});
</script>
