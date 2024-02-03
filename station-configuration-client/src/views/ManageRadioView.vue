<template>
  <v-container>
    <div class="text-h4">Manage Radio</div>
    <div class="text-caption text-medium-emphasis mb-3">Radio ID: {{ radioId }}</div>
    <div v-if="serverError" class="my-3">
      <v-alert type="error" :title="serverError" variant="tonal">
        An error ocurred in accessing the radio's configuration. Please go to the
        <router-link :to="{ name: 'support' }">support page</router-link> and contact support.
      </v-alert>
    </div>
    <div v-else>
      <v-card elevation="3" class="my-4">
        <v-card-title> How To Set Up Your Radio </v-card-title>
        <v-card-text> Video </v-card-text>
      </v-card>

      <v-card v-if="radio" elevation="3" :loading="loadingRadio" class="my-4">
        <v-card-title>Select Stations</v-card-title>
        <v-card-text>
          <div
            v-for="(station, index) in radio.stations"
            :key="station.station_id"
            class="d-flex align-center"
          >
            <div
              class="drop-target pa-1 flex-grow-1"
              :class="station.dragover ? 'highlight' : 'no-hightlight'"
            >
              <div
                class="station pa-2"
                draggable="true"
                @dragover.prevent
                @dragenter.prevent="ondragenter($event, station)"
                @dragleave.prevent="ondragleave($event, station)"
                @drop="ondrop($event, station)"
                @dragstart="ondragstart($event, station)"
              >
                {{ index + 1 }}. {{ station.station_name }}
              </div>
            </div>
            <div class="ml-1">
              <v-btn
                icon="mdi-delete"
                color="red"
                variant="tonal"
                size="small"
                @click="onDeleteStation(station.station_id)"
              ></v-btn>
            </div>
          </div>
          <v-select
            v-if="radio.stations.length < radio.max_station_count"
            v-model="addStationValue"
            :loading="loadingRadio"
            :disabled="loadingRadio"
            variant="outlined"
            class="mt-2 mx-1"
            label="Select A Station To Add"
            :items="availableStations"
            hide-details
            item-title="station_name"
            item-value="station_id"
            @update:model-value="onAddStation"
          ></v-select>
        </v-card-text>
        <v-card-text class="mb-4">
          <div class="text-medium-emphasis font-weight-bold">Tips:</div>
          <div class="mt-2 text-medium-emphasis">
            Drag & drop to reorder stations. Select a station from the list to add it. Settings are
            automatically saved.
          </div>
          <p class="mt-2 text-medium-emphasis">
            On the radio's slider, if one station is selected, any position on the slider will
            select that station. If two are selected, then the slider will be divided in half, if
            three, then thirds, and so on.
          </p>
          <p class="mt-2 text-medium-emphasis">
            Unplug & plug in the radio to load the new settings onto the radio, or wait 24hrs for
            the radio to automatically reload the configuration.
          </p>
        </v-card-text>
      </v-card>
      <v-expansion-panels>
        <v-expansion-panel title="Advanced" elevation="3">
          <v-expansion-panel-text>
            <v-table density="compact">
              <thead>
                <tr>
                  <th class="text-left">Item</th>
                  <th class="text-left">Value</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td>network_id</td>
                  <td>{{ radio.network_id }}</td>
                </tr>
                <tr>
                  <td>pcb_version</td>
                  <td>{{ radio.pcb_version }}</td>
                </tr>
                <tr>
                  <td>firmware_version</td>
                  <td>{{ radio.firmware_version }}</td>
                </tr>
                <tr>
                  <td>max_station_count</td>
                  <td>{{ radio.max_station_count }}</td>
                </tr>
                <tr>
                  <td>has_channel_potentiometer</td>
                  <td>{{ radio.has_channel_potentiometer }}</td>
                </tr>
                <tr>
                  <td>label</td>
                  <td>{{ radio.label }}</td>
                </tr>
                <tr>
                  <td>last_seen</td>
                  <td>{{ convertUTCtoLocal(radio.last_seen) }}</td>
                </tr>
                <tr>
                  <td>show_stations_from_all_networks</td>
                  <td>{{ radio.show_stations_from_all_networks }}</td>
                </tr>
              </tbody>
            </v-table>
          </v-expansion-panel-text>
        </v-expansion-panel>
      </v-expansion-panels>
    </div>
  </v-container>
</template>

<script setup>
import { ref } from 'vue'
import api from '@/api'
const props = defineProps({ radioId: { type: String, require: true, default: null } })
const radioId = props.radioId
const radio = ref(null)
const serverError = ref('')
const loadingRadio = ref(false)
const availableStations = ref([])
const addStationValue = ref(null)

const convertUTCtoLocal = (utcDateTime) => {
  // Assuming the input is in the format 'YYYY-MM-DDTHH:MM:SS'
  // Convert it to a format that JavaScript will correctly parse as UTC
  const correctedDateTime = utcDateTime + 'Z' // Appending 'Z' indicates UTC

  // Create a new Date object, which will automatically convert to local timezone
  const date = new Date(correctedDateTime)

  // Use toLocaleString to format the date. You can customize options as needed
  const options = {
    year: 'numeric',
    month: 'long',
    day: 'numeric',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    hour12: true,
    timeZoneName: 'short'
  }

  // Format the date to a readable string in local timezone
  return date.toLocaleString('en-US', options)
}

fetch(api.radio({ radioId }))
  .then((r) => {
    if (!r.ok) {
      throw `Error: ${r.statusText}`
    }
    return r.json()
  })
  .then((r) => (radio.value = r))
  .then(() => getStations())
  .catch((e) => (serverError.value = e))

const getStations = () => {
  const network_id = radio.value.show_stations_from_all_networks ? null : radio.value.network_id
  fetch(api.stations({ network_id }))
    .then((r) => r.json())
    .then((r) => (availableStations.value = r))
}

const updateRadio = () => {
  const requestOptions = {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      network_id: radio.value.network_id,
      show_stations_from_all_networks: radio.value.show_stations_from_all_networks,
      station_id: radio.value.stations.map((s) => s.station_id)
    })
  }
  loadingRadio.value = true
  fetch(api.radio({ radioId }), requestOptions)
    .then((r) => {
      if (!r.ok) {
        throw `Error: ${r.statusText}`
      }
      return r.json()
    })
    .then((r) => (radio.value = r))
    .catch((e) => (serverError.value = e))
    .finally(() => (loadingRadio.value = false))
}

const moveArrayItem = (arr, fromIndex, toIndex) => {
  // Remove the item from the original position
  const [item] = arr.splice(fromIndex, 1)

  // Insert the item at the new position
  arr.splice(toIndex, 0, item)

  return arr
}

const ondragstart = (e, station) => {
  e.dataTransfer.setData('station_id', station.station_id)
}

const ondragenter = (e, station) => {
  station.dragover = true
}

const ondragleave = (e, station) => {
  station.dragover = false
}

const ondrop = (e, station) => {
  station.dragover = false

  // need to
  const sourceStationID = parseInt(e.dataTransfer.getData('station_id'))
  const targetStationID = station.station_id

  if (sourceStationID == targetStationID) return

  const sourceIndex = radio.value.stations.findIndex((s) => s.station_id === sourceStationID)
  const targetIndex = radio.value.stations.findIndex((s) => s.station_id === targetStationID)

  moveArrayItem(radio.value.stations, sourceIndex, targetIndex)

  updateRadio()
}

const onDeleteStation = (station_id) => {
  const index = radio.value.stations.findIndex((s) => s.station_id === station_id)
  radio.value.stations.splice(index, 1)
  updateRadio()
}

const onAddStation = () => {
  if (addStationValue.value) {
    let station = availableStations.value.find((s) => s.station_id == addStationValue.value)
    radio.value.stations.push({ ...station })
    addStationValue.value = null
    updateRadio()
  }
}
</script>

<style scoped>
.station {
  height: 56px;
  background-color: rgb(24, 103, 192);
  color: white;
  border-radius: 6px;
  line-height: 40px;
  font-size: 1.2rem;
}

.drop-target {
  border: 2px;
  border-style: dashed;
  border-radius: 6px;
}

.drop-target.highlight {
  border-color: rgb(24, 103, 192);
}

.drop-target.no-hightlight {
  border-color: white;
}

.wrap {
  overflow: auto !important;
  text-wrap: wrap !important;
}

:deep(.v-expansion-panel-text__wrapper) {
  padding: 0px;
}
</style>
