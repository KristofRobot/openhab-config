# CO2 Monitor

Integration of TFA Dostmann 'AirCO2ntrol Mini' CO2 Monitor into OpenHAB.

Based on https://hackaday.io/project/5301-reverse-engineering-a-low-cost-usb-co-monitor/log/17909-all-your-base-are-belong-to-us

# Install

    sudo cp co2mon.py /etc/init.d/co2mon.py
    sudo cp 90-co2mini.rules /etc/udev/rules.d/90-co2mini.rules 
    
# Run

    /etc/init.d/co2mon.py /dev/co2mini >> /dev/null &
    
Add the last line to ``/etc/rc.local`` to make it boot safe

# OpenHAB item config

    Number  CO2             "CO2 [%.1f ppm]"   <fire>              
    Number  CO2_T           "Temperature [%.1f °C]"     <temperature>  


