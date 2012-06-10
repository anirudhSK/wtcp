/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.example.hellojni;

import android.app.Activity;
import android.widget.TextView;
import android.widget.Button;
import android.os.Bundle;
import android.widget.EditText;
import android.view.View.* ; 
import android.view.* ;
import android.widget.Toast;
public class HelloJni extends Activity
{

    Button runTest;
    TextView enterIP;
    TextView enterPort;
    EditText destIP;
    EditText destPort;
    public void onButtonClicked(View v) {
        // Do something when the button is clicked
          Toast.makeText(HelloJni.this, "Running UDP ping", Toast.LENGTH_SHORT).show();
          String destIPStr=destIP.getText().toString();
          String destPortStr=destPort.getText().toString();
          if(destIPStr != "" && destPortStr != "" ) {
             runClient(destIPStr,destPortStr);
          }
        }
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        
        enterIP = (TextView) findViewById(R.id.EnterIP);
        enterIP.setText("Enter IP"); 

        enterPort = (TextView) findViewById(R.id.EnterPort);
        enterPort.setText("Enter Port"); 
        
        destIP = (EditText) findViewById(R.id.destIP);
        destIP.setText("128.30.66.123");
        
        destPort = (EditText) findViewById(R.id.destPort);
        destPort.setText("1025");


        /* Create a TextView and set its content.
         * the text is retrieved by calling a native
         * function.
         */
                ////runClient("128.30.66.123","1025");
        //tv.setText( "BBye" );
        //setContentView(tv);

    }
     
    /* A native method that is implemented by the
     * 'hello-jni' native library, which is packaged
     * with this application.
     */
    public native String  stringFromJNI();
    public native void runClient(String destPort,String port);

    /* This is another native method declaration that is *not*
     * implemented by 'hello-jni'. This is simply to show that
     * you can declare as many native methods in your Java code
     * as you want, their implementation is searched in the
     * currently loaded native libraries only the first time
     * you call them.
     *
     * Trying to call this function will result in a
     * java.lang.UnsatisfiedLinkError exception !
     */
    public native String  unimplementedStringFromJNI();

    /* this is used to load the 'hello-jni' library on application
     * startup. The library has already been unpacked into
     * /data/data/com.example.HelloJni/lib/libhello-jni.so at
     * installation time by the package manager.
     */
    static {
        System.loadLibrary("client");
    }
}
