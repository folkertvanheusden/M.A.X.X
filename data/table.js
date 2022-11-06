export const render = (table, rows, action) => {
    // TODO what if now rows? PANIC
    const headers = Object.keys(rows[0])

    const $thead = table.querySelector('thead')
    const $row = $thead.insertRow()

    // headers (TODO split)
    for (const header of headers) {
        const $th = document.createElement('th')
        $th.textContent = header
        $row.append($th)
    }

    // headers (TODO split)
    for (const row of rows) {
        const $row = table.insertRow()

        for (const value of Object.values(row)) {
            const $cell = $row.insertCell()
            $cell.textContent = value
        }

        const $cell = $row.insertCell()
        const $button = document.createElement('a')
        $button.textContent = action.label
        $button.addEventListener('click', () => action.event(row))
        $cell.append($button)
    }
}