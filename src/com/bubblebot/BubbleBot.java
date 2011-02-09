package com.bubblebot;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;

public class BubbleBot extends Activity {

	// Initialize the application
	@Override
	protected void onCreate(Bundle savedInstanceState) {
       super.onCreate(savedInstanceState);
       setContentView(R.layout.main); // Setup the UI
       
       // Hook up handler for form scan button
       Button button01 = (Button) findViewById(R.id.Button01);
       button01.setOnClickListener(new View.OnClickListener() {
           public void onClick(View v) {
        	Intent intent = new Intent(getApplication(), BubbleCollect.class);
   			startActivity(intent); 
           }
       });

       // Hook up handler for form scan button
       Button button02 = (Button) findViewById(R.id.Button02);
       button02.setOnClickListener(new View.OnClickListener() {
           public void onClick(View v) {
        	Intent intent = new Intent(getApplication(), BubbleProcess.class);
   			startActivity(intent); 
           }
       });
	}

	@Override
	protected void onPause() {
		super.onPause();
	}

	@Override
	protected void onResume() {
		super.onResume();
	}
}