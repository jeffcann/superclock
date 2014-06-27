package jeff.com.superclock3150;

import android.app.Activity;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Set;
import java.util.UUID;


public class ClockActivity extends Activity {

    // this is the UUID for Linvor Bluetooth adapter
    private static final UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");
    public static final String BLUETOOTH_DEVICE_NAME = "linvor";

    private BluetoothAdapter bluetooth;
    private BluetoothDevice device;
    private OutputStream out;

    private SimpleDateFormat displayDateFormat = new SimpleDateFormat("d-MMM-yyyy, HH:mm:ss");
    private SimpleDateFormat clockDateFormat = new SimpleDateFormat("d/M/yyyy HH:mm:ss");
    private Long timeDiff = null;

    private TextView txtStatus;
    private TextView txtLocalTime;
    private TextView txtRemoteTime;
    private TextView txtTemperature;
    private TextView txtHumidity;
    private TextView txtLastSync;
    private TextView txtTimeDiff;
    private TextView txtFirmware;
    private TextView txtUptime;
    private Button btnStatus;
    private Button btnSync;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        Log.i("superclock", "Starting up SuperClock App");

        super.onCreate(savedInstanceState);
        setContentView(R.layout.clock_view);

        txtStatus = (TextView) findViewById(R.id.txtStatus);
        txtLocalTime = (TextView) findViewById(R.id.txtLocalTime);
        txtRemoteTime = (TextView) findViewById(R.id.txtRemoteTime);
        txtTemperature = (TextView) findViewById(R.id.txtTemperature);
        txtHumidity = (TextView) findViewById(R.id.txtHumidity);
        txtFirmware = (TextView) findViewById(R.id.txtFirmware);
        txtLastSync = (TextView) findViewById(R.id.txtLastSync);
        txtTimeDiff = (TextView) findViewById(R.id.txtTimeDiff);
        txtUptime = (TextView) findViewById(R.id.txtUptime);

        Button btnConnect = (Button) findViewById(R.id.btnConnect);
        btnConnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                doConnect();
            }
        });

        btnStatus = (Button) findViewById(R.id.btnStatus);
        btnStatus.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                doStatus();
            }
        });

        btnSync = (Button) findViewById(R.id.btnSync);
        btnSync.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                doSync();
            }
        });

        new Thread() {
            public void run() {
                while (true) {
                    updateText(txtLocalTime, displayDateFormat.format(new Date()));

                    if (timeDiff != null) {
                        updateText(txtRemoteTime, displayDateFormat.format(new Date(System.currentTimeMillis() + timeDiff)));
                    }

                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                    }
                }
            }
        }.start();
    }

    /**
     * Convenience method to set the value of a text field. Can be called from any thread,
     * will be executed on the UI thread.
     */
    private void updateText(final TextView txt, final String value) {
        runOnUiThread(new Runnable() {
            public void run() {
                txt.setText(value);
            }
        });
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.clock, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();
        if (id == R.id.action_settings) {
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    /**
     * Called once the phone has successfully connected to the clock
     */
    private void doConnected() {
        runOnUiThread(new Runnable() {
            public void run() {
                updateText(txtStatus, "Connected");
                btnStatus.setEnabled(true);
                btnSync.setEnabled(true);
            }
        });
    }

    /**
     * Synchronise the clock time with the phone time
     */
    private void doSync() {
        String cmd = String.format("s=%s", clockDateFormat.format(new Date()));
        sendCommand(cmd);
    }

    /**
     * Execute a set of commands to get the full status of the clock. Command responses
     * are handled asynchronously in a different thread
     */
    private void doStatus() {
        sendCommand("f"); // firmware version
        sendCommand("d"); // current date/time
        sendCommand("t"); // temperature
        sendCommand("h"); // humidity
        sendCommand("u"); // uptime
        sendCommand("l"); // last sync time
    }

    /**
     * Connect to the device and set up the input/output streams. The device should already
     * be paired.
     */
    private void doConnect() {

        Log.d("superclock", "btnConnect hit");

        updateText(txtStatus, "Getting Bluetooth adapter");
        bluetooth = BluetoothAdapter.getDefaultAdapter();

        updateText(txtStatus, "Turning on Bluetooth");
        Intent turnOn = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
        startActivityForResult(turnOn, 0);
        updateText(txtStatus, "Bluetooth on");

        // search for the clock among the paired devices, just using a hard-coded device name for convenience
        Set<BluetoothDevice> pairedDevices = bluetooth.getBondedDevices();
        for (BluetoothDevice device : pairedDevices) {
            if (device.getName().equals(BLUETOOTH_DEVICE_NAME)) {
                this.device = device;
            }
        }

        if (device != null) {
            updateText(txtStatus, "Found SuperClock");
            new ConnectThread(device).start();
        } else {
            updateText(txtStatus, "SuperClock not found");
        }
    }

    /**
     * Handle responses from the clock
     */
    class CommandHandler {
        public void command(final String cmd, final String data) {
            switch (cmd.charAt(0)) {
                case 'd':
                    Date clockTime = parseClockTime(data);
                    timeDiff = clockTime.getTime() - System.currentTimeMillis();
                    String direction = (timeDiff > 0) ? "fast" : "behind";

                    updateText(txtRemoteTime, displayDateFormat.format(clockTime));
                    updateText(txtTimeDiff, niceTimePeriodFormat(Math.abs(timeDiff)) + " " + direction);
                    break;
                case 'u':
                    updateText(txtUptime, niceTimePeriodFormat(Long.parseLong(data)));
                    break;
                case 't':
                    updateText(txtTemperature, data + (char) 0x00B0);
                    break;
                case 'h':
                    updateText(txtHumidity, data + "%");
                    break;
                case 'f':
                    updateText(txtFirmware, data);
                    break;
                case 'l':
                    long ms = Long.parseLong(data);
                    updateText(txtLastSync, String.format("%s ago", niceTimePeriodFormat(ms)));
                    break;
                case 's':
                    updateText(txtLastSync, "Synchronised!");
                    break;
            }
        }

        private Date parseClockTime(String str) {
            str = str.replace('T', ' ');
            try {
                return clockDateFormat.parse(str);
            } catch (ParseException e) {
                Log.e("superclock", "error parsing date/time", e);
            }
            return null;
        }

        private String niceTimePeriodFormat(long ms) {
            if (ms < 100 * 1000) {
                return String.format("%.1f secs", ms / 1000.0);
            } else if (ms < 60 * 60 * 1000) {
                return String.format("%.1f mins", ms / 60.0 / 1000.0);
            } else if (ms < 24 * 60 * 60 * 1000) {
                return String.format("%.1f hrs", ms / 60.0 / 60.0 / 1000.0);
            } else {
                return String.format("%.1f days", ms / 24.0 / 60.0 / 60.0 / 1000.0);
            }
        }
    }

    /**
     * Buffers incoming text, triggers a command when a new line char is received
     */
    class BufferedCommandInput {

        private StringBuffer input;
        private CommandHandler handler;

        private BufferedCommandInput(CommandHandler handler) {
            this.handler = handler;
            clearBuffer();
        }

        public void readByte(int b) {
            if (b == 10) {
                // ignore
            } else if (b == 13) {
                String command = String.valueOf(input.charAt(0));
                String data = input.substring(2);
                Log.i("superclock.bluetooth", " Received command=" + command + ", data=" + data);
                handler.command(command, data);
                clearBuffer();
            } else {
                input.append((char) b);
            }
        }

        private void clearBuffer() {
            input = new StringBuffer();
        }
    }

    private void manageConnectedSocket(BluetoothSocket socket) {
        InputStream in;
        BufferedCommandInput handler = new BufferedCommandInput(new CommandHandler());

        try {
            in = socket.getInputStream();
            out = socket.getOutputStream();

            Log.d("superclock.bluetooth", "Starting Bluetooth listener");

            while (true) {
                while (in.available() > 0) {
                    int ch = in.read();
                    Log.v("superclock.bluetooth", "RX=" + (char) ch + " (" + ch + ")");
                    handler.readByte(ch);
                }
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                }
            }
        } catch (IOException e) {
            Log.e("superclock.bluetooth", "error with sockets", e);
        }
    }

    private void sendCommand(String command) {
        if (out != null) {
            try {
                out.write((command + "\r\n").getBytes());
            } catch (IOException e) {
                Log.e("superclock.bluetooth", "error while sending command", e);
            }
            try {
                Thread.sleep(50);
            } catch (InterruptedException e) {
            }
        }
    }

    private class ConnectThread extends Thread {
        private final BluetoothSocket mmSocket;

        public ConnectThread(BluetoothDevice device) {
            BluetoothSocket tmp = null;

            try {
                tmp = device.createRfcommSocketToServiceRecord(uuid);
            } catch (IOException e) {
                Log.e("superclock.bluetooth", "error opening Bluetooth socket", e);
            }

            mmSocket = tmp;
        }

        public void run() {
            bluetooth.cancelDiscovery();

            try {
                updateText(txtStatus, "Opening Bluetooth socket");
                mmSocket.connect();
                doConnected();
            } catch (IOException connectException) {
                Log.e("superclock.bluetooth", "error connecting to Bluetooth socket", connectException);

                // Unable to connect; close the socket and get out
                try {
                    mmSocket.close();
                } catch (IOException closeException) {
                    Log.e("superclock.bluetooth", "error closing Bluetooth socket", closeException);
                }
                return;
            }

            manageConnectedSocket(mmSocket);
        }

        public void cancel() {
            try {
                mmSocket.close();
            } catch (IOException e) {
                Log.e("superclock.bluetooth", "error closing Bluetooth socket", e);
            }
        }
    }
}
