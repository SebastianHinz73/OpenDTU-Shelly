export interface ShellyConfig {
    shelly_enable: boolean;
    shelly_moreinfo_enable: boolean;
    shelly_hostname_pro3em: string;
    shelly_hostname_plugs: string;
    limit_enable: boolean;
    max_power: number;
    min_power: number;
    target_value: number;
    feed_in_level: number;
    debug_enable: boolean;
}
