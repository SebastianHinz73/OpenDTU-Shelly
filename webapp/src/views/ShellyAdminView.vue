<template>
    <BasePage :title="$t('shellyadmin.ShellySettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveShellyConfig">
            <CardElement :text="$t('shellyadmin.ShellyConfiguration')" textVariant="text-bg-primary">
                <InputElement :label="$t('shellyadmin.ShellyPro3em')" v-model="shellyConfigList.shelly_hostname_pro3em"
                    type="text" maxlength="128" :placeholder="$t('shellyadmin.HostnameHint')" />

                <InputElement :label="$t('shellyadmin.ShellyPlugS')" v-model="shellyConfigList.shelly_hostname_plugs"
                    type="text" maxlength="128" :placeholder="$t('shellyadmin.HostnameHint')" />

                <InputElement :label="$t('shellyadmin.PollInterval')" v-model="shellyConfigList.pollinterval" type="number"
                    min="0" max="15" :postfix="$t('shellyadmin.Seconds')" />

                <InputElement :label="$t('shellyadmin.MaxPower')" v-model="shellyConfigList.max_power" type="number" min="1"
                    max="3000" :placeholder="$t('shellyadmin.MaxPowerHint')" />

                <InputElement :label="$t('shellyadmin.LimitPower')" v-model="shellyConfigList.limit_power" type="number"
                    min="1" max="3000" :placeholder="$t('shellyadmin.LimitPowerHint')" />
            </CardElement>
            <FormFooter @reload="getShellyConfig" />
        </form>
    </BasePage>
</template>

<script lang="ts">
import BasePage from '@/components/BasePage.vue';
import BootstrapAlert from "@/components/BootstrapAlert.vue";
import CardElement from '@/components/CardElement.vue';
import FormFooter from '@/components/FormFooter.vue';
import InputElement from '@/components/InputElement.vue';
import type { ShellyConfig } from "@/types/ShellyConfig";
import { authHeader, handleResponse } from '@/utils/authentication';
import { BIconInfoCircle } from 'bootstrap-icons-vue';
import { defineComponent } from 'vue';

export default defineComponent({
    components: {
        BasePage,
        BootstrapAlert,
        CardElement,
        FormFooter,
        InputElement,
        BIconInfoCircle,
    },
    data() {
        return {
            dataLoading: true,
            shellyConfigList: {} as ShellyConfig,
            alertMessage: "",
            alertType: "info",
            showAlert: false,
        };
    },
    created() {
        this.getShellyConfig();
    },
    methods: {
        getShellyConfig() {
            this.dataLoading = true;
            fetch("/api/shelly/config", { headers: authHeader() })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (data) => {
                        this.shellyConfigList = data;
                        this.dataLoading = false;
                    }
                );
        },
        saveShellyConfig(e: Event) {
            e.preventDefault();

            const formData = new FormData();
            formData.append("data", JSON.stringify(this.shellyConfigList));

            fetch("/api/shelly/config", {
                method: "POST",
                headers: authHeader(),
                body: formData,
            })
                .then((response) => handleResponse(response, this.$emitter, this.$router))
                .then(
                    (response) => {
                        this.alertMessage = this.$t('apiresponse.' + response.code, response.param);
                        this.alertType = response.type;
                        this.showAlert = true;
                    }
                );
        },
    },
});
</script>
