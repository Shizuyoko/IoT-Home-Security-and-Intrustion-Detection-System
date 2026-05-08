import smtplib
import sys

device = sys.argv[1]
event_type = sys.argv[2]
location = sys.argv[3]

sender = "evanbrowne82@gmail.com"
receiver = "ncto nhih dlwc osob"

# Gmail App Password
password = "YOUR_APP_PASSWORD"


subject = "ESP32 Alert"

body = f"""
Alert detected

Device: {device}
Event: {event_type}
Location: {location}
"""

message = f"Subject: {subject}\n\n{body}"


try:
    server = smtplib.SMTP(
        "smtp.gmail.com",
        587
    )

    server.starttls()

    server.login(
        sender,
        password
    )

    server.sendmail(
        sender,
        receiver,
        message
    )

    server.quit()

    print("Email sent")

except Exception as e:
    print("Email failed:", e)