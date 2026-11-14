/*
 * dmod in a browser - the page as dmodOS shows it, without dmodOS: open the page in a browser to design it.
 *
 * On the device the page is a view shown by dmview and the application is on the other side of the channel: dmod.prop / get / set /
 * onchange / emit are dmview's properties and events, compiled into the view. The conversion does not run this file. Here it plays the
 * part of the application: it counts the taps.
 */
(function () {
    const props = {};
    const listeners = {};
    let taps = 0;

    function set(name, value) {
        if (props[name] === value) return;
        props[name] = value;
        (listeners[name] || []).forEach(fn => fn());
    }

    window.dmod = {
        prop(name, initial) { if (!(name in props)) props[name] = initial; return props[name]; },
        get(name) { return props[name]; },
        set(name, value) { props[name] = value; },
        onchange(name, fn) { (listeners[name] = listeners[name] || []).push(fn); },
        emit(name, value) {
            console.log('emit', name, value === undefined ? 0 : value);
            if (name === 'tap') set('taps', 'taps: ' + (++taps));
        },
    };
})();
