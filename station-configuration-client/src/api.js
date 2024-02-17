import { apiHost } from '@/settings'

const apiPrefix = `${apiHost}/api/v1`

const api = {
  admins: `${apiPrefix}/admins`,
  adminsSessions: `${apiPrefix}/admins/sessions`,
  stations: ({ network_id = null }) => {
    return network_id
      ? `${apiPrefix}/networks/stations?network_id=${network_id}`
      : `${apiPrefix}/networks/stations`
  },
  station: ({ stationId }) => `${apiPrefix}/networks/stations/${stationId}`,
  radios: `${apiPrefix}/radios`,
  radio: ({ radioId }) => `${apiPrefix}/radios/${radioId}`,
  radioDeviceInterfaceV1_0: ({ radioId }) => `${apiPrefix}/radios/device_interface/v1.0/${radioId}`
}

export default api
