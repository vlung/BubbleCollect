package com.bubblebot;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

public class DisplayProcessedData extends Activity{
	
	TextView data;
	File dir = new File("/sdcard/BubbleBot/processedText/");

	String filename = "";
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
       super.onCreate(savedInstanceState);
       setContentView(R.layout.processed_data);
       
       Bundle extras = getIntent().getExtras(); 
       if(extras !=null)
       {
    	   filename = extras.getString("file") + ".txt";
       }
       
       //read in captured data from textfile
	   File file = new File(dir,filename);
       
	   //Read text from file
	   StringBuilder text = new StringBuilder();
	   text.append(dir.toString() + "/" + filename + "\n\n");
	   try {
		   BufferedReader br = new BufferedReader(new FileReader(file));
		   String line;
	
		   while ((line = br.readLine()) != null) 
		   {
	         	text.append(line);
	         	text.append('\n');
		   }
	   }
	   catch (IOException e) {
		   //You'll need to add proper error handling here
	   }

       data = (TextView) findViewById(R.id.text);
       data.setText("Data from: " + text);
	}

}
