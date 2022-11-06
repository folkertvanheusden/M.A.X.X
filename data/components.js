import { $ } from './dom.js'

export const meter = value => `<meter min="0" max="100" value="${value}" low="50" high="70" optimum="100">${value}</meter>`

export const snack = (message, context) => {
    $('.message').textContent = message
    $('.message').classList.remove('text-danger')
    if (context) {
        $('.message').classList.add(context)
    }
    setTimeout(() => $('.message').textContent = '', 5000)
}

export const properties = (dl, properties) => {
    for (const [name, value] of Object.entries(properties)) {
        const dt = document.createElement('dt')
        const dd = document.createElement('dd')
        dt.textContent = name
        dd.textContent = value
        dl.append(dt, dd)
    }
}
export const table = (table, rows, action, caption = '') => {
    table.innerHTML = `
        <caption>${caption}</caption>
        <thead></thead>
        <tbody></tbody>
        <tfoot></tfoot>
    `

    const headers = Object.keys(rows[0] || {})

    const $tbody = table.querySelector('tbody')
    const $thead = table.querySelector('thead')
    const $tfoot = table.querySelector('tfoot')

    // head
    const $row = $thead.insertRow()
    for (const header of headers) {
        const $th = document.createElement('th')
        $th.textContent = header
        $row.append($th)
    }

    // action placeholder
    $row.append(document.createElement('th'))

    // body
    for (const row of rows) {
        const $row = $tbody.insertRow()

        for (const [name, value] of Object.entries(row)) {
            const $cell = $row.insertCell()
            if (name === 'strength') {
                $cell.innerHTML = value
            } else {
                $cell.textContent = value
            }
        }

        const $cell = $row.insertCell()
        const $button = document.createElement('a')
        $button.textContent = action.label
        $button.addEventListener('click', () => action.event(row))
        $cell.append($button)
    }

    // foot
    const $footrow = $tfoot.insertRow()
    const $footcell = $footrow.insertCell()
    $footcell.textContent = `${rows.length} row${rows.length != 1 ? 's' : ''}`
}
