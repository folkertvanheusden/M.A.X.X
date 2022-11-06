export const Method = {
    Get: 'GET',
    Post: 'POST',
    Delete: 'DELETE',
}

export const request = async (url, method = Method.Get, data) => {
    const headers = { 'Content-Type': 'application/json' }
    const body = JSON.stringify(data)

    const response = await fetch(url, { method, headers, body })

    if (!response.ok) {
        throw new Error(`${url}: ${response.status} ${response.statusText}`)
    }

    return response.json()
}
