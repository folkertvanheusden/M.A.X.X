import { request, Method } from './fetch.js'

export const AuthMode = [
    'OPEN',
    'WEP',
    'WPA-PSK',
    'WPA2-PSK',
    'WPA-WPA2-PSK',
    'WPA2-ENTERPRISE',
    'WPA3-PSK',
    'WPA2-WPA3-PSK',
    'WAPI-PSK',
    'OWE',
    'MAX',
]

const endpoint = '/api/wifi'

const configured = async () => request(`${endpoint}/configlist`)

const scan = async () => request(`${endpoint}/scan`)

const status = async () => request(`${endpoint}/status`)

const add = async (apName, apPass) => request(`${endpoint}/add`, Method.Post, { apName, apPass })

const deleteById = async id => request(`${endpoint}/id`, Method.Post, { id })

const deleteByApName = async apName => request(`${endpoint}/apName`, Method.Post, { apName })

export const actions = {
    start: async () => request(`${endpoint}/softAp/stop`, Method.Post),
    scan:  async () => request(`${endpoint}/scan`),
}

export { configured, scan, status, add, deleteById, deleteByApName }
