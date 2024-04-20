<template>
    <BasePage :title="$t('shellyadmin.ShellySettings')" :isLoading="dataLoading">
        <BootstrapAlert v-model="showAlert" dismissible :variant="alertType">
            {{ alertMessage }}
        </BootstrapAlert>

        <form @submit="saveShellyConfig">

            <CardElement :text="$t('shellyadmin.ShellyGet')" textVariant="text-bg-primary">
                <InputElement :label="$t('shellyadmin.ShellyGetSwitch')" v-model="shellyConfigList.shelly_enable"
                    type="checkbox" />

                <InputElement :label="$t('shellyadmin.ShellyPro3em')" v-model="shellyConfigList.shelly_hostname_pro3em"
                    type="text" maxlength="128" :placeholder="$t('shellyadmin.HostnameHint')"
                    v-show="shellyConfigList.shelly_enable" />

                <InputElement :label="$t('shellyadmin.ShellyPlugS')" v-model="shellyConfigList.shelly_hostname_plugs"
                    type="text" maxlength="128" :placeholder="$t('shellyadmin.HostnameHint')"
                    v-show="shellyConfigList.shelly_enable" />

                <InputElement :label="$t('shellyadmin.ShellyMoreInfoSwitch')"
                    v-model="shellyConfigList.shelly_moreinfo_enable" type="checkbox"
                    v-show="shellyConfigList.shelly_enable" />
            </CardElement>

            <CardElement :text="$t('shellyadmin.ShellySet')" textVariant="text-bg-primary" add-space
                v-show="shellyConfigList.shelly_enable">

                <InputElement :label="$t('shellyadmin.ShellySetSwitch')" v-model="shellyConfigList.limit_enable"
                    type="checkbox" v-show="shellyConfigList.shelly_enable" />

                <InputElement :label="$t('shellyadmin.ShellyDebugSwitch')" v-model="shellyConfigList.debug_enable"
                    type="checkbox" v-show="shellyConfigList.shelly_enable && shellyConfigList.limit_enable" />

                <InputElement :label="$t('shellyadmin.MaxPower')" v-model="shellyConfigList.max_power" type="number"
                    min="1" max="3000" :placeholder="$t('shellyadmin.MaxPowerHint')"
                    v-show="shellyConfigList.shelly_enable && shellyConfigList.limit_enable" />

                <InputElement :label="$t('shellyadmin.MinPower')" v-model="shellyConfigList.min_power" type="number"
                    min="0" max="500" :placeholder="$t('shellyadmin.MinPowerHint')"
                    v-show="shellyConfigList.shelly_enable && shellyConfigList.limit_enable" />

                <InputElement :label="$t('shellyadmin.TargetValue')" v-model="shellyConfigList.target_value"
                    type="number" min="-100" max="300" :placeholder="$t('shellyadmin.TargetValueHint')"
                    v-show="shellyConfigList.shelly_enable && shellyConfigList.limit_enable" />

                <div class="row mb-3" v-if="shellyConfigList.limit_enable">
                    <label for="inputFeedInLevel" class="col-sm-2 col-form-label">
                        {{ $t('shellyadmin.ZeroFeedInLevel') }}
                        <BIconInfoCircle v-tooltip :title="$t('shellyadmin.ZeroFeedInLevelHint')" />
                    </label>
                    <div class="col-sm-10">
                        <div class="input-group mb-3">
                            <input type="range" class="form-control form-range" v-model="shellyConfigList.feed_in_level"
                                min="0" max="100" id="inputFeedInLevel" aria-describedby="basic-addon1"
                                style="height: unset;" />
                            <span class="input-group-text" id="basic-addon1">{{ FeedInLevelText }}</span>
                        </div>
                    </div>
                </div>
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
    computed: {
        FeedInLevelText() {
            return this.$t("shellyadmin.Percent", { per: this.$n(this.shellyConfigList.feed_in_level * 1) });
        },
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
