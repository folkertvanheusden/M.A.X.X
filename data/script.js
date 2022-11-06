import * as Api from './api.js'
import { $, $$ } from './dom.js'
import { meter, properties, snack, table } from './components.js'


// TODO decouple gui <-> api

addEventListener('unhandledrejection', e => snack(e.reason, 'text-danger'))

const actions = $$('button')
actions.forEach(action =>
    action.addEventListener('click', async () => {
        const result = await Api.actions[action.dataset.value]()
        snack(result.message, '')
    })
)

// fetch data
const [configured, scanned, status] = await Promise.all([
    Api.configured(),
    Api.scan(),
    Api.status(),
])


// status

const $status = $('.status dl')
properties($status, status)



// TODO are saved networks available as well? there is no rssi for saved networks, so fetch it from available?


// saved networks

const mapper1 = l => ({
    name: l.apName,
    password: l.apPass ? 'yes' : 'no',
    id: l.id,
})


const lastUpdate = t => {
    return (new Date()).toLocaleTimeString()
    const diff = (new Date() - t) / (60 * 1000)

    const formatter = new Intl.RelativeTimeFormat('en', { style: 'long' })
    return formatter.format(diff, 'minute')
}


const renderSaved = configured => table($('.saved'), configured.map(mapper1), {
    label: 'remove',
    event: async row => {
        const result = await Api.deleteById(row.id)
        snack(result.message, '')

        renderSaved(await Api.configured())
    }
}, `Last update ${lastUpdate()}`)
setTimeout(async () => renderSaved(await Api.configured()), 1 * 1500)
renderSaved(configured)


// available networks

const mapper2 = l => ({
    strength: meter(127 + parseInt(l.rssi)),
    ssid: l.ssid,
    encryption: Api.AuthMode[l.encryptionType],
    encrypted: l.encryptionType > 0 ? '🔒\ufe0e' : '',
    channel: l.channel,
    rssi: l.rssi + ' dbm',
})


const renderScanned = scanned => table($('.available'), scanned.map(mapper2), {
    // TODO see https://github.com/MartinVerges/womolin-waterlevel/blob/main/ui/src/routes/wifi.svelte
    //          (it has 2 modi; 1) add unknown ssid + password (if any) 2) add ssid + password for a known network
    label: 'add',
    event: async row => {
        const apName = row.ssid
        const apPass = prompt(`Password for ${apName}`)

        const result = await Api.add(apName, apPass)
        snack(result.message, 'text')

        renderScanned(await Api.scan())
    }
}, `Last update ${lastUpdate()}`)

// TODO handle scanning (test file: scanning, 200 ok) vs list (test file: scan, 200 ok)
// simulate by copying contents of file 'scanning' to file 'scan'
// currently not an issue (the list will be loaded eventually after a few retries)
setInterval(async () => renderScanned(await Api.scan()), 5 * 60 * 1000)
renderScanned(scanned)
