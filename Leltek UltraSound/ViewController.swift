//
//  ViewController.swift
//  Leltek UltraSound
//
//  Created by wiza on 2017/9/21.
//  Copyright (c) 2017 Leltek. All rights reserved.
//

import UIKit
import Photos

class ViewController: UIViewController {

    var ultraSoundImageView: UIImageView?
    var ultraSoundImageView_PartialScreen: UIImageView?
    var ultraSoundImageView_FullScreen: UIImageView?

    var statusLine: UILabel?

    /** navigation bar-------------------------
     Cliff 0108
     */
    var naviBar : UINavigationBar?
    @objc var buttonAdd:UIBarButtonItem?
    var naviTitle = "ultraSound" as String?
    var organ = "General" as String?
    var setting = "0.0" as String?
    var patientNo: String?
    var bedNo: String?
    
    var rate = 0.0 as Float?
    var calLine: UILabel?
    // navigation bar---------------------------
    /** button image------------------------------
    Cliff 0110
    */
    var image1none, image2none, image3none, image1, image2, image3 : UIButton?
    var buttonArchive : UIButton?
    //
    var floatL1 = 0.0 as Float?
    var floatL2 = 0.0 as Float?
    var floatL3 = 0.0 as Float?
    var floatVol = 0.0 as Float?
    var stringL1 = "0" as String?
    var stringL2 = "0" as String?
    var stringL3 = "0" as String?
    var stringVol = "0" as String?
    //--------------------------
    
    var buttonPage = 0
    var buttonMaxPage = 1
    var buttonPadLayout = false
    var fullScreen = false
    var buttonBottomEdgeInVertical: CGFloat = 0

    var buttonSwitch, buttonPageLeft, buttonPageRight: UIButton?
    var buttonColor, buttonMMode, buttonGainUp, buttonGainDown, buttonPowerUp, buttonPowerDown: UIButton?
    var buttonAnnotate, buttonContrast, buttonSave, buttonRulerLine, buttonRulerEllipse, buttonRulerClear: UIButton?
    var buttonFullScreen, buttonPartialScreen: UIButton?

    var sliderHistory: UISlider?
    var sliderHistory_PartialScreen: UISlider?
    var sliderHistory_FullScreen: UISlider?

    var sliderContrast, sliderBrightness: UISlider?
    var labelContrast: UILabel?
    var buttonContrastReset: UIButton?

    var edittextAnnotate: UITextField?

    var timer: Timer?
    var device_on = false
    var device_ever_started = false
    var waitingStart = 0 //1:initializing, 2:success, -1:fail
    var cycleCount = 0
    var AnnotationEditingIndex: Int32 = 0
    var saveImageNextRender = false

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view, typically from a nib.

        self.view.isMultipleTouchEnabled = true
        //self.view.backgroundColor = UIColor(red: 0.125, green: 0.125, blue: 0.125, alpha: 1)
        self.view.backgroundColor = .black

        if UIScreen.main.nativeBounds.height == 2436  //iPhone X
         {
            buttonBottomEdgeInVertical = 20
         }
        else
         {
            //LelSetResolutionDiv (2)   //use lower resolution for speed
         }

        //LelSetRulerVolumeMeasurement (0.6)

        //Create All UI objects
        prepareUI ((self.view?.bounds.size)! )

        //Create Timer
         timer = Timer.scheduledTimer(timeInterval: 0.05, target: self, selector: #selector(self.cycle), userInfo: nil, repeats: true)

        //Receive notification when APP resign active
        let app = UIApplication.shared
        NotificationCenter.default.addObserver(self, selector: #selector(ViewController.applicationWillResignActive(notification:)), name: NSNotification.Name.UIApplicationWillResignActive, object: app)

    }

    @objc
    func applicationWillResignActive(notification: NSNotification) {
        if device_on {
            buttonAction(sender: buttonSwitch)
            
        }
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }

        func handleTouch (_ act: Int, _ touches: Set<UITouch>, _ event: UIEvent?)
         {
            var idx: Int = 0, x: Int = 0, y: Int = 0, x2: Int = 0, y2: Int = 0

            for touch in touches
              {
                 if touch.view == ultraSoundImageView
                  {
                     idx = idx + 1
                     let currentPoint = touch.location(in: ultraSoundImageView)
                     if idx == 1
                      {  x = Int (currentPoint.x);  y = Int (currentPoint.y); }
                     else if idx ==  2
                      {  x2 = Int (currentPoint.x);  y2 = Int (currentPoint.y); }
                  }
              } //for

             if  idx > 0
             {
               LelTouch (CInt (act), CInt (idx), CInt (x), CInt (y), CInt (x2), CInt (y2))
             }

            annotationSelectCheck ()
            ShowUltraSoundImage ()
         }


    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
         handleTouch (0, touches, event)
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
         handleTouch (1, touches, event)
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouch (2, touches, event)
    }
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        handleTouch (2, touches, event)
    }


    override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
        if UIDevice.current.orientation.isLandscape {
            //print("Landscape")
        } else {
            //print("Portrait")
        }

        prepareUI (size)

    }



    @objc func cycle ()
    {
       //waiting Initialization?
       if waitingStart != 0
        {
            if waitingStart == 2  //success
            {
                buttonSwitch!.setTitle ("Freeze", for: .normal)
                buttonSwitch!.backgroundColor = .black
                moveTitleToBottom (buttonSwitch!)
                sliderHistory?.value =  Float (LelGetHistoryMax ())
                sliderHistory?.isHidden=true

                device_on = true
                device_ever_started = true

                buttonSwitch!.isEnabled = true
                waitingStart = 0
            }
            else if waitingStart == -1  //failure
            {
                buttonSwitch!.backgroundColor = .black

                let alertController = UIAlertController(title: "Leltek Ultrasound", message: "Cannot Connect Device.", preferredStyle: UIAlertControllerStyle.alert)
                alertController.addAction(UIAlertAction(title: "OK", style: UIAlertActionStyle.default, handler: nil))
                present(alertController, animated: true, completion: nil)

                buttonSwitch!.isEnabled = true
                waitingStart = 0
            }
        } //if waitingStart


       LelCycle () //NOTE: device may be stopped, call this to get ruler information at then

       //for unknown reason image cannot be shown when initializing
       if waitingStart == 0
        {
            ShowUltraSoundImage ()
        }
        else if cycleCount % 4 == 0
        {
          if buttonSwitch?.backgroundColor == .blue
          { buttonSwitch?.backgroundColor = .black }
          else
          { buttonSwitch?.backgroundColor = .blue }
        }

        cycleCount = cycleCount + 1
    }


         func moveTitleToBottom (_ button: UIButton)
         {
             if ( button.currentImage == nil )
               { return }

             button.titleEdgeInsets = UIEdgeInsetsMake(button.currentImage!.size.height + 2, -button.currentImage!.size.width, 0, 0);
             button.imageEdgeInsets = UIEdgeInsetsMake(-button.titleLabel!.intrinsicContentSize.height, 0, 0, -button.titleLabel!.intrinsicContentSize.width);
         }

         func setUIVisibility ()
         {
           //hide/display button
            var hidden: Bool

            if fullScreen
             {
               buttonSwitch?.isHidden=true
               buttonFullScreen?.isHidden=true
               buttonPartialScreen?.isHidden=false

               ultraSoundImageView_FullScreen?.isHidden=false
               ultraSoundImageView_PartialScreen?.isHidden=true
               ultraSoundImageView = ultraSoundImageView_FullScreen

               sliderHistory_FullScreen?.isHidden=(device_on || !device_ever_started)
               sliderHistory_PartialScreen?.isHidden=true
               sliderHistory_FullScreen!.value=sliderHistory_PartialScreen!.value
               sliderHistory = sliderHistory_FullScreen;
             }
            else
             {
               buttonSwitch?.isHidden=false
               buttonFullScreen?.isHidden=false
               buttonPartialScreen?.isHidden=true

               ultraSoundImageView_FullScreen?.isHidden=true
               ultraSoundImageView_PartialScreen?.isHidden=false
               ultraSoundImageView = ultraSoundImageView_PartialScreen

               sliderHistory_FullScreen?.isHidden=true
               sliderHistory_PartialScreen?.isHidden=(device_on || !device_ever_started)
               sliderHistory_PartialScreen!.value=sliderHistory_FullScreen!.value
               sliderHistory = sliderHistory_PartialScreen
             }

            //Page 0
            if fullScreen
              { hidden = true }
            else if  buttonPadLayout == true && buttonPage == 0
              { hidden = false }
            else if  buttonPadLayout == false && buttonPage == 0
              { hidden = false }
            else
              { hidden = true }
            buttonColor?.isHidden=hidden
            buttonMMode?.isHidden=hidden
            buttonGainUp?.isHidden=hidden
            buttonGainDown?.isHidden=hidden
            buttonPowerUp?.isHidden=hidden
            buttonPowerDown?.isHidden=hidden

            //Page 1
            if fullScreen
              { hidden = true }
            else if  buttonPadLayout == true && buttonPage == 0
              { hidden = false }
            else if  buttonPadLayout == false && buttonPage == 1
              { hidden = false }
            else
              { hidden = true }
            buttonAnnotate?.isHidden=hidden
            buttonContrast?.isHidden=hidden
            buttonSave?.isHidden=hidden
            buttonRulerLine?.isHidden=hidden
            buttonRulerEllipse?.isHidden=hidden
            buttonRulerClear?.isHidden=hidden

            
            // custom 0112 Cliff
            isimageload()
            //don't show image btn in full screen
            
            
            //PageUp/Down
            if ( buttonPage > 0 && !fullScreen )
              { buttonPageLeft?.isHidden=false }
            else
              { buttonPageLeft?.isHidden=true }

            if ( buttonPage < buttonMaxPage  && !fullScreen )
              { buttonPageRight?.isHidden=false }
            else
              { buttonPageRight?.isHidden=true }

         }

         func setButtonByMarkMode ()
         {
            let currmode = LelGetMarkMode ()

            if ( currmode == LelMarkMode_Line )
             { buttonRulerLine?.backgroundColor = .blue }
            else
             { buttonRulerLine?.backgroundColor = .black }

            if ( currmode == LelMarkMode_Ellipse )
             { buttonRulerEllipse?.backgroundColor = .blue }
            else
             { buttonRulerEllipse?.backgroundColor = .black }

            if ( currmode == LelMarkMode_Annotate )
             {
               buttonAnnotate?.backgroundColor = .blue
               edittextAnnotate?.isHidden=false
             }
            else
             {
               buttonAnnotate?.backgroundColor = .black
               edittextAnnotate?.isHidden=true
             }
         }

         func annotationSelectCheck ()
         {
           if ( LelGetLastAnnotationHolding () !=  AnnotationEditingIndex )
            {
              AnnotationEditingIndex = Int32 (LelGetLastAnnotationHolding ())
              if (AnnotationEditingIndex != 0)
              {
                 edittextAnnotate?.text =  String(cString: LelGetAnnotationText (AnnotationEditingIndex))
              }
              else
               { edittextAnnotate?.text = "" }
            }
         }
        func annotationTextChanged ()
         {
                if ( ultraSoundImageView == nil || edittextAnnotate == nil ||
                    ultraSoundImageView!.frame.width <= 0 )
                 { return }

                let str: String? = edittextAnnotate!.text
                let str2 = str!.cString(using: String.Encoding.utf8)
                let strc = UnsafeMutablePointer<Int8>(mutating: str2)

                if ( AnnotationEditingIndex ==  0 )
                  {
                     if ( str != "" )
                      {
                          AnnotationEditingIndex = LelSetAnnotation (0, strc!)
                          if ( AnnotationEditingIndex != 0 )
                           {
                             LelPutAnnotation ( CInt (AnnotationEditingIndex),
                                     CInt (ultraSoundImageView!.frame.width/2),
                                     CInt (ultraSoundImageView!.frame.height/2),
                                     0, 0, 1);
                           }
                      }
                  }
                 else
                  {
                    LelSetAnnotation (AnnotationEditingIndex, strc!);
                  }
         }
        func clearRulersSub (withAnnotations act:Bool)
         {
                if ( act )
                 { LelClearAllAnnotations() }

                LelClearAllRulers ()
                annotationSelectCheck ()
                ShowUltraSoundImage ()

         }

        func switchFullScreen (_ b:Bool)
         {
           if ( b == fullScreen )
           {  return }

           fullScreen = b
           setUIVisibility ()
           LelSetViewSize ( CInt((ultraSoundImageView?.frame.width)!), CInt ((ultraSoundImageView?.frame.height)!) )
           ShowUltraSoundImage ()
         }



    func prepareUI (_ size: CGSize)
    {

        var upperspace: CGFloat = UIApplication.shared.statusBarFrame.height
        //Cliff 0108 to fit navi Bar
        if upperspace < 44
          {  upperspace = 44}


        //Create UI objects if none
        //----------------------------------------
        
        /**
         Cliff 0108 navigation bar-----------------------------
         */
        if naviBar ==  nil
        {
            let naviBar: UINavigationBar = UINavigationBar (frame: CGRect(x: 0, y: 0, width: size.width, height: 44))
            self.view.addSubview(naviBar);
            readFromPlist();
            let naviItem = UINavigationItem(title: naviTitle!);
            let addItem = UIBarButtonItem(barButtonSystemItem: UIBarButtonSystemItem.add, target: self, action: #selector(ViewController.plusBtnTouched));
            let cleanItem = UIBarButtonItem(barButtonSystemItem: UIBarButtonSystemItem.trash, target: self, action: #selector(ViewController.cleanBtnTouched))
            naviItem.rightBarButtonItem = addItem;
            naviItem.leftBarButtonItem = cleanItem;
            naviBar.setItems([naviItem], animated: false);
        }

        //naviBar----------------------------------------------
        
        //Main View
        if  ultraSoundImageView_PartialScreen == nil
          {
            let imageView = UIImageView()
            imageView.frame = CGRect(x: 0, y: upperspace, width: size.width, height: size.height-upperspace)
            imageView.isUserInteractionEnabled = true //Enable touch
            imageView.isMultipleTouchEnabled = true
            //imageView.layer.borderWidth = CGFloat(5);
            //imageView.layer.borderColor = UIColor.lightGray.cgColor
            self.view.addSubview(imageView)
            ultraSoundImageView_PartialScreen = imageView
            ultraSoundImageView = ultraSoundImageView_PartialScreen
         }
        if  ultraSoundImageView_FullScreen == nil
          {
            let imageView = UIImageView()
            imageView.frame = CGRect(x: 0, y: upperspace, width: size.width, height: size.height-upperspace)
            imageView.isUserInteractionEnabled = true //Enable touch
            imageView.isMultipleTouchEnabled = true
            //imageView.layer.borderWidth = CGFloat(5);
            //imageView.layer.borderColor = UIColor.lightGray.cgColor
            self.view.addSubview(imageView)
            ultraSoundImageView_FullScreen = imageView
         }



        //Status Line
        if statusLine == nil
         {
            let label = UILabel(frame: CGRect(x: 0, y: 50, width: (self.view?.bounds.size.width)!, height: 20))
            //label.center = CGPoint(x: 160, y: 50)
            label.textAlignment = .left
            label.textColor = UIColor.white
            label.text = "(Status)"
            self.view.addSubview(label)
            statusLine = label
         }
        // Cliff CalLine 0112-----
        if calLine == nil
        {
            let label = UILabel (frame: CGRect(x: 0, y: 75, width: (self.view?.bounds.size.width)!, height: 20))
            label.textAlignment = .left
            label.textColor = UIColor.white
            label.text = setCalLabel()
            self.view.addSubview(label)
            calLine = label
        }
        //------------------------
        //create buttons
        if buttonSwitch == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Start", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/switch.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button); buttonSwitch = button;
         }
        if buttonPageLeft == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("<", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonPageLeft = button;
         }
        if buttonPageRight == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle(">", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonPageRight = button;
         }
        if buttonColor == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Color", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/coloroff.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonColor = button;
         }
        if buttonMMode == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("MMode", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/mmode.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonMMode = button;
         }
        if buttonGainUp == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Gain+", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/gainup.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonGainUp = button;
         }
        if buttonGainDown == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Gain-", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/gaindown.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonGainDown = button;
         }
        if buttonPowerUp == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Power+", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/powerup.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonPowerUp = button;
         }
        if buttonPowerDown == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Power-", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/powerdown.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonPowerDown = button;
         }
        if buttonAnnotate == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Annotate", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/annotate.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonAnnotate = button;
         }
        if buttonContrast == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Contrast", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/contrast.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonContrast = button;
         }
        if buttonSave == nil
         {
             let button = UIButton()
             button.backgroundColor = .black;  button.setTitle("Save", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/save_image.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button);  buttonSave = button;
         }
        if buttonRulerLine == nil
         {
             let button = UIButton();   button.backgroundColor = .black
             button.setTitle("Length", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/line.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button); buttonRulerLine = button;
         }
        if buttonRulerEllipse == nil
         {
             let button = UIButton();   button.backgroundColor = .black
             button.setTitle("Area", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/ellipse.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button); buttonRulerEllipse = button;
         }
        if buttonRulerClear == nil
         {
             let button = UIButton();   button.backgroundColor = .black
             button.setTitle("Clear", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/eraser.png")!, for: UIControlState.normal)
             moveTitleToBottom (button); self.view.addSubview(button); buttonRulerClear = button;
         }
        if buttonFullScreen == nil
         {
             let button = UIButton();   button.backgroundColor = .black
             button.setTitle("", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/fullscreen.png")!, for: UIControlState.normal)
             button.backgroundColor = UIColor.black.withAlphaComponent(0)
             moveTitleToBottom (button); self.view.addSubview(button); buttonFullScreen = button;
         }
        if buttonPartialScreen == nil
         {
             let button = UIButton();   button.backgroundColor = .black
             button.setTitle("", for: .normal)
             button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
             button.setImage(UIImage(named: "image/partialscreen.png")!, for: UIControlState.normal)
             button.backgroundColor = UIColor.black.withAlphaComponent(0)
             moveTitleToBottom (button); self.view.addSubview(button); buttonPartialScreen = button;
         }
        /**
         Cliff 0110 add three image icon------------------------------------------------------
         */
        if image1none == nil{
            let button = UIButton();
            button.setTitle("", for: .normal)
            button.setImage(UIImage(named: "image/red.png")!, for: UIControlState.normal)
            button.backgroundColor = UIColor.black.withAlphaComponent(0)
            moveTitleToBottom(button); self.view.addSubview(button);
            image1none = button
        }
        if image2none == nil{
            let button = UIButton();
            button.setTitle("", for: .normal)
            button.setImage(UIImage(named: "image/red.png")!, for: UIControlState.normal)
            button.backgroundColor = UIColor.black.withAlphaComponent(0)
            moveTitleToBottom(button); self.view.addSubview(button);
            image2none = button
        }
        if image3none == nil{
            let button = UIButton();
            button.setTitle("", for: .normal)
            button.setImage(UIImage(named: "image/red.png")!, for: UIControlState.normal)
            button.backgroundColor = UIColor.black.withAlphaComponent(0)
            moveTitleToBottom(button); self.view.addSubview(button);
            image3none = button
        }
        if image1 == nil{
            let button = UIButton();
            button.setTitle("", for: .normal)
            button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
            button.setImage(UIImage(named: "image/green.png")!, for: UIControlState.normal)
            button.backgroundColor = UIColor.black.withAlphaComponent(0)
            moveTitleToBottom(button); self.view.addSubview(button);
            image1 = button
        }
        if image2 == nil{
            let button = UIButton();
            button.setTitle("", for: .normal)
            button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
            button.setImage(UIImage(named: "image/green.png")!, for: UIControlState.normal)
            button.backgroundColor = UIColor.black.withAlphaComponent(0)
            moveTitleToBottom(button); self.view.addSubview(button);
            image2 = button
        }
        if image3 == nil{
            let button = UIButton();
            button.setTitle("", for: .normal)
            button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
            button.setImage(UIImage(named: "image/green.png")!, for: UIControlState.normal)
            button.backgroundColor = UIColor.black.withAlphaComponent(0)
            moveTitleToBottom(button); self.view.addSubview(button);
            image3 = button
        }
        if buttonArchive == nil{
            let button = UIButton();
            button.setTitle("", for :.normal)
            button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
            button.setImage(UIImage(named: "image/archive.png"), for: UIControlState.normal)
            button.backgroundColor = UIColor.black.withAlphaComponent(0)
            moveTitleToBottom(button); self.view.addSubview(button);
            buttonArchive = button
        }
        // add image icon --------------------------------------------------------------------------

        //slider
        if sliderHistory_PartialScreen == nil
         {
            let slider = UISlider (frame: CGRect(x: 100, y: 100, width: 100, height: 50))
            slider.backgroundColor = UIColor.black.withAlphaComponent(0.3)
            slider.maximumTrackTintColor = .red
            slider.minimumTrackTintColor = .blue
            slider.thumbTintColor = .white
            slider.minimumValue = 0
            slider.maximumValue = Float (LelGetHistoryMax ())
            slider.value =  Float (LelGetHistoryMax ())
            slider.isContinuous = true
            slider.isHidden=true
            slider.addTarget (self, action: #selector(onSliderChange),  for: UIControlEvents.valueChanged)
            self.view.addSubview(slider)
            sliderHistory_PartialScreen = slider
            sliderHistory = sliderHistory_PartialScreen
         }
        if sliderHistory_FullScreen == nil
         {
            let slider = UISlider (frame: CGRect(x: 100, y: 100, width: 100, height: 50))
            slider.backgroundColor = UIColor.black.withAlphaComponent(0.3)
            slider.maximumTrackTintColor = .red
            slider.minimumTrackTintColor = .blue
            slider.thumbTintColor = .white
            slider.minimumValue = 0
            slider.maximumValue = Float (LelGetHistoryMax ())
            slider.value =  Float (LelGetHistoryMax ())
            slider.isContinuous = true
            slider.isHidden=true
            slider.addTarget (self, action: #selector(onSliderChange),  for: UIControlEvents.valueChanged)
            self.view.addSubview(slider)
            sliderHistory_FullScreen = slider
         }

        //annotate
        if edittextAnnotate == nil
         {
            edittextAnnotate = UITextField  (frame: CGRect(x: 100, y: 100, width: 100, height: 50))
            edittextAnnotate?.placeholder = "Annotate"
            edittextAnnotate?.textColor = UIColor.black
            edittextAnnotate?.backgroundColor = UIColor.white
            edittextAnnotate?.addTarget(self, action: #selector(textFieldDidChange(_:)), for: .editingChanged)
            self.view.addSubview(edittextAnnotate!)
            edittextAnnotate?.isHidden=true
         }

        //conrast/brightness
        if sliderContrast == nil
         {
            let slider = UISlider (frame: CGRect(x: 100, y: 100, width: 100, height: 50))
            slider.backgroundColor = UIColor.black.withAlphaComponent(0.3)
            slider.maximumTrackTintColor = .cyan
            slider.minimumTrackTintColor = .cyan
            slider.thumbTintColor = .white
            slider.minimumValue = 0
            slider.maximumValue = Float (10)
            slider.value =  0
            slider.isContinuous = true
            slider.isHidden=true
            slider.addTarget (self, action: #selector(onSliderChange),  for: UIControlEvents.valueChanged)
            self.view.addSubview(slider)
            sliderContrast = slider
         }
        if sliderBrightness == nil
         {
            let slider = UISlider (frame: CGRect(x: 100, y: 100, width: 100, height: 50))
            slider.backgroundColor = UIColor.black.withAlphaComponent(0.3)
            slider.maximumTrackTintColor = .cyan
            slider.minimumTrackTintColor = .cyan
            slider.thumbTintColor = .white
            slider.minimumValue = Float (-10)
            slider.maximumValue = Float (10)
            slider.value =  0
            slider.isContinuous = true
            slider.isHidden=true
            slider.addTarget (self, action: #selector(onSliderChange),  for: UIControlEvents.valueChanged)
            self.view.addSubview(slider)
            sliderBrightness = slider
         }
        if labelContrast == nil
         {
            let label = UILabel()
            label.text = "Contr./Bright."
            label.textAlignment = .center
            label.textColor = .white
            label.backgroundColor = .black
            label.isHidden=true
            self.view.addSubview(label)
            labelContrast=label
         }
        if buttonContrastReset == nil
         {
            let button = UIButton()
            button.backgroundColor = UIColor.gray
            button.setTitle("Reset", for: .normal)
            button.addTarget(self, action: #selector(buttonAction), for: .touchUpInside)
            button.isHidden=true
            self.view.addSubview(button)
            buttonContrastReset = button
         }


        // Layout
        //----------------------------

        var bw, bh, bspace, bsumw, bsumh, bline, bpage: CGFloat;  //button size
        var bx, by: CGFloat;



        if  size.width > 600 && size.height > 600  //Pad
         {
           buttonPadLayout = true
           buttonMaxPage = 0
           bspace = 5;
           bline = 80;  //height of scrollbar, status line
           bpage = 80;  //width of pageup/pagedown button
         }
        else  //Phone
         {
           buttonPadLayout = false
           buttonMaxPage = 1
           bspace = 2;
           bline = 50;  //height of scrollbar, status line
           bpage = 50;  //width of pageup/pagedown button
         }

        if  size.width > size.height    //Horizontal Layout (Landscape)
         {
            if buttonPadLayout
             {
               bh = (size.height - upperspace - bspace*7) / 8;  bw = bh;
               bsumw = bw * 2 + bpage * 2 + bspace * 3;
               bsumh = bh * 8 + bspace * 7;

               ultraSoundImageView_PartialScreen?.frame = CGRect(x: bsumw, y: upperspace, width: (size.width-bsumw), height: (size.height)-upperspace)
               ultraSoundImageView_FullScreen?.frame = CGRect(x: 0, y: upperspace, width: (size.width), height: (size.height)-upperspace)

               statusLine?.frame =  CGRect(x: bsumw, y: upperspace, width: (size.width-bsumw), height: bline )
               edittextAnnotate?.frame =  CGRect(x: bsumw + bline, y: upperspace + bline, width: (size.width-bsumw-bline*2-bw*3/2), height: bline )
                //Cliff 0112
                calLine?.frame = CGRect(x: bsumw, y: upperspace+bline/3, width: (size.width-bsumw), height: bline )
                

               sliderContrast?.frame =  CGRect(x: bsumw + bline, y: upperspace + bline*4, width: (size.width-bsumw-bline*2)/3, height: bline )
               labelContrast?.frame =  CGRect(x: bsumw + bline+(size.width-bsumw-bline*2)/3, y: upperspace + bline*4,
                              width: (size.width-bsumw-bline*2)/3, height: bline )
               buttonContrastReset?.frame =  CGRect(x: bsumw + bline+(size.width-bsumw-bline*2)/3, y: upperspace + bline*5,
                              width: (size.width-bsumw-bline*2)/3, height: bline )
               sliderBrightness?.frame =  CGRect(x: bsumw + bline+(size.width-bsumw-bline*2)*2/3, y: upperspace + bline*4,
                              width: (size.width-bsumw-bline*2)/3, height: bline )

               sliderHistory_PartialScreen?.frame = CGRect(x: bsumw, y: size.height-bline, width: (size.width-bsumw), height: bline )
               sliderHistory_FullScreen?.frame = CGRect(x: 0, y: size.height-bline, width: (size.width), height: bline )

               buttonFullScreen?.frame = CGRect(x:(size.width-bw*5/4), y:upperspace+bline/2, width:bw, height:bh);
               buttonPartialScreen?.frame = CGRect(x:(size.width-bw*5/4), y:upperspace+bline/2, width:bw, height:bh);

               bx = 0; by = upperspace;
               buttonPageLeft?.frame = CGRect(x: bx, y: by, width: bpage, height: bsumh - bspace)

               bx += bpage ; by = upperspace;
               buttonSwitch?.frame = CGRect(x: bx, y: by, width: bw*2+bspace, height: bh)
               by += bh + bspace
               buttonColor?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonGainDown?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonPowerDown?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonAnnotate?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonContrast?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonSave?.frame = CGRect(x: bx, y: by, width: bw, height: bh)

               bx += bw + bspace ; by = upperspace;
               by += bh + bspace;
               buttonMMode?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonGainUp?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonPowerUp?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace;
               buttonRulerLine?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace;
               buttonRulerEllipse?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace;
               buttonRulerClear?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
                bx += bw + bspace ; by = upperspace;
                buttonPageRight?.frame = CGRect(x: bx, y: by, width: bpage, height: bsumh - bspace)
               /**
                 Cliff 0110 add image icon----------------------------------------
                 */
                bx += bw + bspace; by = upperspace;
                image1?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image1none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                by += bh + bspace;
                image2?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image2none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                by += bh + bspace;
                image3?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image3none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                by += bh + bspace;
                buttonArchive?.frame = CGRect(x: bx, y:by, width:bw/2, height:bh/3)
                //add image icon---------------------------------------------------
               
             } //Pad-Horizontal
            else  //Phone-Horizontal
             {
               bh = (size.height - upperspace - bspace*4) / 5;  bw = bh;
               bsumw = bw * 2 + bpage * 2 + bspace * 3;
               bsumh = bh * 5 + bspace * 4;

               ultraSoundImageView_PartialScreen?.frame = CGRect(x: bsumw, y: upperspace, width: (size.width-bsumw), height: (size.height)-upperspace)
               ultraSoundImageView_FullScreen?.frame = CGRect(x: 0, y: upperspace, width: (size.width), height: (size.height)-upperspace)

               statusLine?.frame =  CGRect(x: bsumw, y: upperspace, width: (size.width-bsumw), height: bline )
               edittextAnnotate?.frame =  CGRect(x: bsumw + bline, y: upperspace + bline, width: (size.width-bsumw-bline*2-bw*3/2), height: bline )
                calLine?.frame = CGRect(x: bsumw, y: upperspace+bline/3, width: (size.width-bsumw), height: bline )


               sliderContrast?.frame =  CGRect(x: bsumw + bline, y: upperspace + bline*4, width: (size.width-bsumw-bline*2)/3, height: bline )
               labelContrast?.frame =  CGRect(x: bsumw + bline+(size.width-bsumw-bline*2)/3, y: upperspace + bline*4,
                              width: (size.width-bsumw-bline*2)/3, height: bline )
               buttonContrastReset?.frame =  CGRect(x: bsumw + bline+(size.width-bsumw-bline*2)/3, y: upperspace + bline*5,
                              width: (size.width-bsumw-bline*2)/3, height: bline )
               sliderBrightness?.frame =  CGRect(x: bsumw + bline+(size.width-bsumw-bline*2)*2/3, y: upperspace + bline*4,
                              width: (size.width-bsumw-bline*2)/3, height: bline )

               sliderHistory_PartialScreen?.frame = CGRect(x: bsumw, y: size.height-bline, width: (size.width-bsumw), height: bline )
               sliderHistory_FullScreen?.frame = CGRect(x: 0, y: size.height-bline, width: (size.width), height: bline )

               buttonFullScreen?.frame = CGRect(x:(size.width-bw*5/4), y:upperspace+bline/2, width:bw, height:bh);
               buttonPartialScreen?.frame = CGRect(x:(size.width-bw*5/4), y:upperspace+bline/2, width:bw, height:bh);

               bx = 0; by = upperspace;
               buttonPageLeft?.frame = CGRect(x: bx, y: by, width: bpage, height: bsumh - bspace)

               bx += bpage ; by = upperspace;
               buttonSwitch?.frame = CGRect(x: bx, y: by, width: bw*2+bspace, height: bh)
               by += bh + bspace
               buttonColor?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonAnnotate?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonGainDown?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonContrast?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonPowerDown?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonSave?.frame = CGRect(x: bx, y: by, width: bw, height: bh)

               bx += bw + bspace ; by = upperspace;
               by += bh + bspace;
               buttonMMode?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonRulerLine?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonGainUp?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonRulerEllipse?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               by += bh + bspace
               buttonPowerUp?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonRulerClear?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
                
                bx += bw + bspace ; by = upperspace;
                buttonPageRight?.frame = CGRect(x: bx, y: by, width: bpage, height: bsumh - bspace)
                /**
                 Cliff 0110 add image icon----------------------------------------
                 */
                bx += bw + bspace; by = upperspace
                by += bh + bspace;
                image1none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image1?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)

                by += bh + bspace;
                image2none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image2?.frame = CGRect(x:bx, y:by ,width:bw/2, height:bh/3)

                by += bh + bspace;
                image3none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image3?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                by += bh + bspace;
                buttonArchive?.frame = CGRect(x: bx, y:by, width:bw/2, height:bh/3)

                //add image icon---------------------------------------------------

             } //Phone-Horizontal
         }
        else   //Vertical Layout
         {
            if buttonPadLayout
             {
               bw = (size.width - bpage * 2 - bspace*8) / 7; bh = bw;
               bsumw = bw * 7 + bpage * 2 + bspace * 8;
               bsumh = bh * 2 + bspace * 2;

               ultraSoundImageView_PartialScreen?.frame = CGRect(x: 0, y: upperspace, width: (size.width), height: (size.height)-upperspace-bsumh)
               ultraSoundImageView_FullScreen?.frame = CGRect(x: 0, y: upperspace, width: (size.width), height: (size.height)-upperspace)

               statusLine?.frame =  CGRect(x: 0, y: upperspace, width: (size.width), height: bline )
               edittextAnnotate?.frame =  CGRect(x:  bline, y: upperspace + bline, width: (size.width-bline*2-bw*3/2), height: bline )
                calLine?.frame = CGRect(x: 0, y: upperspace+bline/3, width: (size.width), height: bline )
                edittextAnnotate?.frame =  CGRect(x: bsumw + bline, y: upperspace + bline, width: (size.width-bsumw-bline*2-bw*3/2), height: bline )

               sliderContrast?.frame =  CGRect(x: bline, y: upperspace + bline*4, width: (size.width-bline*2)/3, height: bline )
               labelContrast?.frame =  CGRect(x: bline+(size.width-bline*2)/3, y: upperspace + bline*4,
                              width: (size.width-bline*2)/3, height: bline )
               buttonContrastReset?.frame =  CGRect(x: bline+(size.width-bline*2)/3, y: upperspace + bline*5,
                              width: (size.width-bline*2)/3, height: bline )
               sliderBrightness?.frame =  CGRect(x: bline+(size.width-bline*2)*2/3, y: upperspace + bline*4,
                              width: (size.width-bline*2)/3, height: bline )

               sliderHistory_PartialScreen?.frame = CGRect(x: 0, y: size.height-bsumh - bline, width: (size.width), height: bline )
               sliderHistory_FullScreen?.frame = CGRect(x: 0, y: size.height-bline, width: (size.width), height: bline )

               buttonFullScreen?.frame = CGRect(x:(size.width-bw*5/4), y:upperspace+bline/2, width:bw, height:bh);
               buttonPartialScreen?.frame = CGRect(x:(size.width-bw*5/4), y:upperspace+bline/2, width:bw, height:bh);

               bx = 0; by = (size.height)-bsumh
               buttonPageLeft?.frame = CGRect(x: bx, y: by, width: bpage, height: bsumh - bspace)

               bx = bpage + bspace;
               buttonSwitch?.frame = CGRect(x: bx, y: by, width: bw, height: bsumh - bspace)
               bx += bw + bspace
               buttonColor?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonGainUp?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonPowerUp?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonAnnotate?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonContrast?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonSave?.frame = CGRect(x: bx, y: by, width: bw, height: bh)

               bx += bw + bspace
               buttonPageRight?.frame = CGRect(x: bx, y: by, width: bpage, height: bsumh - bspace)

               bx = bpage + bspace;
               by += bw + bspace;

               bx += bw + bspace
               buttonMMode?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonGainDown?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonPowerDown?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonRulerLine?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonRulerEllipse?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonRulerClear?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
                /**
                 Cliff 0110 add image icon----------------------------------------
                 */
                bx = bpage + bspace ; by = (size.height)-bsumh-(bspace*36);
                
                
                bx += bw + bspace;
                image1none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image1?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                
                bx += bw + bspace;
                image2none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image2?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                
                bx += bw + bspace;
                image3none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image3?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                
                bx += bw + bspace;
                buttonArchive?.frame = CGRect(x: bx, y:by, width:bw/2, height:bh/3)
                
                //add image icon---------------------------------------------------

             }  //Pad-Vertical
            else //Phone-Vertical
             {
               bw = (size.width - bpage * 2 - bspace*5) / 4; bh = bw;
               bsumw = bw * 4 + bpage * 2 + bspace * 5;
               bsumh = bh * 2 + bspace * 2 + buttonBottomEdgeInVertical

               ultraSoundImageView_PartialScreen?.frame = CGRect(x: 0, y: upperspace, width: (size.width), height: (size.height)-upperspace-bsumh)
               ultraSoundImageView_FullScreen?.frame = CGRect(x: 0, y: upperspace, width: (size.width), height: (size.height)-upperspace)

               statusLine?.frame =  CGRect(x: 0, y: upperspace, width: (size.width), height: bline )
               edittextAnnotate?.frame =  CGRect(x:  bline, y: upperspace + bline, width: (size.width-bline*2-bw*3/2), height: bline )
                
                calLine?.frame = CGRect(x: 0, y: upperspace+bline/3, width: (size.width), height: bline )
                edittextAnnotate?.frame =  CGRect(x: bsumw + bline, y: upperspace + bline, width: (size.width-bsumw-bline*2-bw*3/2), height: bline )

               sliderContrast?.frame =  CGRect(x: bline, y: upperspace + bline*4, width: (size.width-bline*2)/3, height: bline )
               labelContrast?.frame =  CGRect(x: bline+(size.width-bline*2)/3, y: upperspace + bline*4,
                              width: (size.width-bline*2)/3, height: bline )
               buttonContrastReset?.frame =  CGRect(x: bline+(size.width-bline*2)/3, y: upperspace + bline*5,
                              width: (size.width-bline*2)/3, height: bline )
               sliderBrightness?.frame =  CGRect(x: bline+(size.width-bline*2)*2/3, y: upperspace + bline*4,
                              width: (size.width-bline*2)/3, height: bline )

               sliderHistory_PartialScreen?.frame = CGRect(x: 0, y: size.height-bsumh - bline, width: (size.width), height: bline )
               sliderHistory_FullScreen?.frame = CGRect(x: 0, y: size.height-bline, width: (size.width), height: bline )

               buttonFullScreen?.frame = CGRect(x:(size.width-bw*5/4), y:upperspace+bline/2, width:bw, height:bh);
               buttonPartialScreen?.frame = CGRect(x:(size.width-bw*5/4), y:upperspace+bline/2, width:bw, height:bh);

               bx = 0; by = (size.height)-bsumh
               buttonPageLeft?.frame = CGRect(x: bx, y: by, width: bpage, height: bsumh - bspace)

               bx = bpage + bspace;
               buttonSwitch?.frame = CGRect(x: bx, y: by, width: bw, height: bsumh - bspace)
               bx += bw + bspace
               buttonColor?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonAnnotate?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonGainUp?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonContrast?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonPowerUp?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonSave?.frame = CGRect(x: bx, y: by, width: bw, height: bh)

               bx += bw + bspace
               buttonPageRight?.frame = CGRect(x: bx, y: by, width: bpage, height: bsumh - bspace)

               bx = bpage + bspace;
               by += bw + bspace;

               bx += bw + bspace
               buttonMMode?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonRulerLine?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonGainDown?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonRulerEllipse?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               bx += bw + bspace
               buttonPowerDown?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
               buttonRulerClear?.frame = CGRect(x: bx, y: by, width: bw, height: bh)
                /**
                 Cliff 0110 add image icon----------------------------------------
                 */
                bx = bpage + bspace ; by = (size.height)-bsumh-(bspace*36);
                
                bx += bw + bspace;
                image1none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image1?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                
                bx += bw + bspace;
                image2none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image2?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                
                bx += bw + bspace;
                image3none?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                image3?.frame = CGRect(x:bx, y:by, width:bw/2, height:bh/3)
                
                bx += bw + bspace;
                buttonArchive?.frame = CGRect(x: bx, y:by, width:bw/2, height:bh/3)
                
                //add image icon---------------------------------------------------

             }  //Phone-Vertical
         }

         setUIVisibility ();


        LelSetViewSize ( CInt((ultraSoundImageView?.frame.width)!), CInt ((ultraSoundImageView?.frame.height)!) )



    }
    
    /**
     Cliff 0108 bar item add Data--------------------------------------------
     */
    @objc func plusBtnTouched() {
        self.present(AddDataViewController(), animated: true, completion: nil)
    }
    @objc func cleanBtnTouched(){
        let fileManager = FileManager.default
        let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        let path = documentDirectory.appending("/example.plist")
        do{
        if fileManager.fileExists(atPath: path){
            try fileManager.removeItem(atPath: path)
            }
        }catch {
             print("failed")
            }
        let path1 = documentDirectory.appending("/"+naviTitle!+"_"+String(1)+".png")
        do{
            if fileManager.fileExists(atPath: path1){
                try fileManager.removeItem(atPath: path1)
               
            }
        }catch {
            print("failed")
        }
        let path2 = documentDirectory.appending("/"+naviTitle!+"_"+String(2)+".png")
        do{
            if fileManager.fileExists(atPath: path2){
                try fileManager.removeItem(atPath: path2)
            }
        }catch {
            print("failed")
        }
        let path3 = documentDirectory.appending("/"+naviTitle!+"_"+String(3)+".png")
        do{
            if fileManager.fileExists(atPath: path3){
                try fileManager.removeItem(atPath: path3)
            }
        }catch {
            print("failed")
        }
        }
    // add config-------------------------------------------------------------


    /**
     Cliff 0110 read from plist----------------------------------------------
     */
    func readFromPlist(){
        
        
        let fileManager = FileManager.default
        let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        let path = documentDirectory.appending("/example.plist")
        if fileManager.fileExists(atPath: path){
            let dataDict = NSDictionary(contentsOfFile: path)
            if let dict = dataDict{
                patientNo = dict.object(forKey: "Patient No") as? String
                organ = dict.object(forKey: "Organ")as? String
                bedNo = dict.object(forKey: "Bed No") as? String
                setting = dict.object(forKey: "Setting") as? String
                
                naviTitle = "Patient No: "+patientNo!+" Organ: "+organ!
                print("patient No:",patientNo!)
                print("Setting", setting!)
            }
            else{
                naviTitle = "UltraSound"
                print("load failed")
            }
        }
    }
    // read from plist -----------------------------------------------------
    

     @objc func buttonAction(sender: UIButton!) {
        if sender == buttonSwitch
         {
            if !device_on
            {
                let queue = DispatchQueue(label:"com.leltek.ultrasound_ios", qos:.background)

                sender.isEnabled = false
                sender.backgroundColor = .blue
                statusLine?.text = "Connecting Device..."

                waitingStart = 1;

                queue.async {

                    //DO NOT perform any UI action here: it will not work

                    let result = LelStart ()

                    //it is found that accessing sender here will be ignored

                    if result != 0  //success
                    {
                        self.waitingStart = 2; //success
                    }
                    else //initialization fail
                    {
                        self.waitingStart = -1; //fail
                    }

                    //following operation will be handled in cycle ()
                    //for you cannot access UI elements in 2nd thread


                } //queue

                return //release UI thread
            }
            else //if device_on
             {
                LelStop ()
                device_on = false
                sender.setTitle ("Continue", for: .normal)
                sender.backgroundColor =  UIColor(red: 0, green: 0.5, blue: 0, alpha: 1)
                moveTitleToBottom (sender)

                sliderHistory?.isHidden=false

                sliderHistory?.value =  Float (LelGetHistoryMax ())

             }
         }

        else if sender == buttonPageLeft
         {
           if ( buttonPage > 0 )
            {
               buttonPage -= 1
               setUIVisibility ()
            }
         }

        else if sender == buttonPageRight
         {
           if ( buttonPage < buttonMaxPage )
            {
               buttonPage += 1
               setUIVisibility ()
            }
         }

        else if  sender == buttonContrast
         {
            if sender.backgroundColor == .black
             {
                sender.backgroundColor = .blue
                sliderContrast?.isHidden=false
                sliderBrightness?.isHidden=false
                labelContrast?.isHidden=false
                buttonContrastReset?.isHidden=false
             }
            else
             {
                sender.backgroundColor = .black
                sliderContrast?.isHidden=true
                sliderBrightness?.isHidden=true
                labelContrast?.isHidden=true
                buttonContrastReset?.isHidden=true
             }

         }
        else if  sender == buttonContrastReset
         {
           sliderContrast?.value = 0
           sliderBrightness?.value = 0
           LelSetPixelValueAdd(0)
           LelSetPixelValueScale (1)
           ShowUltraSoundImage ()
         }
        else if  sender == buttonRulerLine
         {
             if  LelGetMarkMode () != LelMarkMode_Line
              {
                LelSetMarkMode (LelMarkMode_Line)
              }
             else
              {
                LelSetMarkMode (LelMarkMode_None)
              }
             setButtonByMarkMode ()
         }
        else if  sender == buttonRulerEllipse
         {
             if LelGetMarkMode () != LelMarkMode_Ellipse
              {
                LelSetMarkMode (LelMarkMode_Ellipse)
              }
             else
              {
                LelSetMarkMode (LelMarkMode_None)
              }
             setButtonByMarkMode ()
         }
        else if  sender == buttonRulerClear
         {
             if ( LelCountUserAnnotation () > 0)
              {
                 let alert = UIAlertController(title: "Clear More?", message: "Also Remove Annotations?", preferredStyle: .alert)

                 alert.addAction(UIAlertAction(title: "Keep", style: .default, handler: {
                     (action: UIAlertAction!) -> Void in
                       self.clearRulersSub(withAnnotations: false)
                    }))
                 alert.addAction(UIAlertAction(title: "Remove Annotations", style: .default, handler: {
                    (action: UIAlertAction!) -> Void in
                    self.clearRulersSub(withAnnotations: true)
                    }))
                 self.present(alert, animated: true)
              }
             else
              {
                 LelClearAllRulers ()
                 annotationSelectCheck ()
                 ShowUltraSoundImage ()
              }



         }
        else if  sender == buttonAnnotate
         {
             if  LelGetMarkMode () != LelMarkMode_Annotate
              {
                LelSetMarkMode (LelMarkMode_Annotate)
              }
             else
              {
                LelSetMarkMode (LelMarkMode_None)
              }
             setButtonByMarkMode ()
         }
        else if sender == buttonSave
         {
            if ultraSoundImageView != nil
            {
              saveImageNextRender = true

              ShowUltraSoundImage ()

              saveImageNextRender = false
                
              //let activityItem: [AnyObject] = [ultraSoundImageView!.image as AnyObject]
              
                //save Cliff 0111
                saveImageDocumentDirectory()
              
              // let avc = UIActivityViewController(activityItems: activityItem as [AnyObject], applicationActivities: nil)

              //present(avc, animated: true, completion: nil)
                
            }
         }
        else if sender == buttonFullScreen
        {
            switchFullScreen (true)
        }
        else if sender == buttonPartialScreen
        {
            switchFullScreen (false)
        }

        else if sender == buttonMMode
        {
            if sender.backgroundColor == .black && LelSetMMode (1) != 0
            {
                sender.backgroundColor = .blue
            }
            else if  LelSetMMode (0) != 0
            {
                sender.backgroundColor = .black
            }
            annotationSelectCheck ()
        }

        else if device_on
         {
              if sender == buttonColor
               {
                 if sender.backgroundColor == .black && LelSetColorMode (1) != 0
                  {
                    sender.backgroundColor = .blue
                    sender.setImage(UIImage(named: "image/color.png")!, for: UIControlState.normal)
                  }
                 else if  LelSetColorMode (0) != 0
                  {
                    sender.backgroundColor = .black
                    sender.setImage(UIImage(named: "image/coloroff.png")!, for: UIControlState.normal)
                  }
               }

              else if sender == buttonGainUp
               {
                 if sender.backgroundColor != .blue
                  {
                    if ( LelSetGain (1) != 0 )
                      { buttonGainUp?.backgroundColor = .blue }
                  }
                 else
                  {
                    if ( LelSetGain (0) != 0 )
                      { buttonGainUp?.backgroundColor = .black }
                  }
               }
              else if sender == buttonGainDown
               {
                 if ( LelSetGain (0) != 0 )
                   { buttonGainUp?.backgroundColor = .black }
               }
              else if sender == buttonPowerUp
               {
                 if sender.backgroundColor != .blue
                  {
                    if ( LelSetPower (1) != 0 )
                      { buttonPowerUp?.backgroundColor = .blue }
                  }
                 else
                  {
                    if ( LelSetPower (0) != 0 )
                      { buttonPowerUp?.backgroundColor = .black }
                  }
               }
              else if sender == buttonPowerDown
               {
                 if ( LelSetPower (0) != 0 )
                   { buttonPowerUp?.backgroundColor = .black }
               }

         }
            
        /** Cliff 0112 image control
             */
        else if sender == image1
        {
            let fileManager = FileManager.default
            let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
            let path = documentDirectory.appending("/"+naviTitle!+"_"+String(1)+".png")
            if (fileManager.fileExists(atPath: path)) {
                self.present(Image1ViewController(), animated: true, completion: nil)
            }
            else{
                return
            }
        }
        else if sender == image2
        {
            let fileManager = FileManager.default
            let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
            let path = documentDirectory.appending("/"+naviTitle!+"_"+String(2)+".png")
            if (fileManager.fileExists(atPath: path)) {
                self.present(Image2ViewController(), animated: true, completion: nil)
            }
            else{
                return
            }
        }
            else if sender == image3
            {
                let fileManager = FileManager.default
                let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
                let path = documentDirectory.appending("/"+naviTitle!+"_"+String(3)+".png")
                if (fileManager.fileExists(atPath: path)) {
                    self.present(Image3ViewController(), animated: true, completion: nil)
                }
                else{
                    return
                }
            }
        else if sender == buttonArchive
        {
            // Cliff Archive
            archiveplist()
            compressfile()
            deletefile()
            //call avc
        }
        // Cliff 0112 button control
    }

     @objc func onSliderChange (_ sender:UISlider) {

         if sender == sliderHistory
          {
              LelReadFromHistory (CInt (sliderHistory!.value))
              ShowUltraSoundImage ()
          }
         else if sender == sliderContrast
          {
              LelSetPixelValueScale (CFloat(1.0 + sliderContrast!.value * 0.2))
              ShowUltraSoundImage ()
          }
         else if sender == sliderBrightness
          {
              LelSetPixelValueAdd (CFloat(sliderBrightness!.value * 5))
              ShowUltraSoundImage ()
          }
    }


    @objc func textFieldDidChange(_ textField: UITextField) {
         if textField == edittextAnnotate
          { annotationTextChanged () }
    }


        func ShowUltraSoundImage () {

            if ultraSoundImageView == nil
              { return }

            // Create CGImage from Display Buffer
            //----------------------------------------
            let bufsize = Int (LelGetDisplayBufferSize ())
            let bufw = Int (LelGetDisplayBufferWidth())
            let bufh = Int (LelGetDisplayBufferHeight())
            let buf = Data (bytes: UnsafeRawPointer (LelGetDisplayBuffer ()), count: bufsize)
            let zscale = Float (LelGetZoomScale ())

            let cfdata = NSData(data: buf) as CFData
            let provider: CGDataProvider! = CGDataProvider(data: cfdata)
            if provider == nil {
                print("CGDataProvider is not supposed to be nil")
                return //nil
            }
            let bufimage: CGImage! = CGImage(
                width: bufw,
                height: bufh,
                bitsPerComponent: 8,
                bitsPerPixel: 32,
                bytesPerRow: bufw * 4 ,
                space: CGColorSpaceCreateDeviceRGB(),
                bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedFirst.rawValue),
                provider: provider,
                decode: nil,
                shouldInterpolate: true,
                intent: .defaultIntent
            )
            if bufimage == nil {
                print("CGImage is not supposed to be nil")
                return //nil
            }

            // Create Scaled CGImage to fit view
            //----------------------------------------
            let resultw = ultraSoundImageView!.frame.width
            let resulth = ultraSoundImageView!.frame.height
            var bufimage2:UIImage? = nil

            if zscale <= 1.0
              { bufimage2 = UIImage(cgImage:bufimage!) }
            else
            {
                let panArea = CGRect (x:CGFloat (LelGetZoomLeft ()), y:CGFloat(LelGetZoomUp ()),
                                      width:CGFloat(Float(bufw)/zscale), height:CGFloat(Float(bufh)/zscale))
                bufimage2 = UIImage(cgImage:bufimage.cropping(to: panArea)!)
            }

            //Copy ultrasound image onto buffer
            UIGraphicsBeginImageContext(CGSize(width:resultw, height:resulth))
             bufimage2?.draw(in: CGRect(origin: CGPoint.zero, size: CGSize(width: resultw, height: resulth)))

            //Device Status & Annotations
            var resultshort = resultw
            if ( resultshort > resulth )
               { resultshort = resulth}
            var textFontSize = resultshort/20
            if buttonPadLayout
             { textFontSize = resultshort/30 }

            let textFont = UIFont(name: "Arial-BoldMT", size: textFontSize)!
            let textattrBlack = [
                NSAttributedStringKey.font: textFont,
                NSAttributedStringKey.foregroundColor: UIColor.black
                ] as [NSAttributedStringKey : Any]
            let textattrWhite = [
                NSAttributedStringKey.font: textFont,
                NSAttributedStringKey.foregroundColor: UIColor.white
                ] as [NSAttributedStringKey : Any]
            let textattrCyan = [
                NSAttributedStringKey.font: textFont,
                NSAttributedStringKey.foregroundColor: UIColor.cyan
                ] as [NSAttributedStringKey : Any]


            textFontSize = resultshort/22
            if buttonPadLayout
             { textFontSize = resultshort/32 }
            let dev_small_textFont = UIFont(name: "Arial-BoldMT", size: textFontSize)!
            let dev_small_textattrBlack = [
                NSAttributedStringKey.font: dev_small_textFont,
                NSAttributedStringKey.foregroundColor: UIColor.black
                ] as [NSAttributedStringKey : Any]
            let dev_small_textattrGray = [
                NSAttributedStringKey.font: dev_small_textFont,
                NSAttributedStringKey.foregroundColor: UIColor.gray
                ] as [NSAttributedStringKey : Any]
            let dev_small_textattrRed = [
                NSAttributedStringKey.font: dev_small_textFont,
                NSAttributedStringKey.foregroundColor: UIColor.red
                ] as [NSAttributedStringKey : Any]

            textFontSize = resultshort/18
            if buttonPadLayout
             { textFontSize = resultshort/27 }
            let dev_textFont = UIFont(name: "Arial-BoldMT", size: textFontSize)!
            let dev_textattrBlack = [
                NSAttributedStringKey.font: dev_textFont,
                NSAttributedStringKey.foregroundColor: UIColor.black
                ] as [NSAttributedStringKey : Any]
            let dev_textattrGray = [
                NSAttributedStringKey.font: dev_textFont,
                NSAttributedStringKey.foregroundColor: UIColor.gray
                ] as [NSAttributedStringKey : Any]

            //Devices
            var devx, devy: CGFloat
            devx = CGFloat(resultw * 19 / 20)
            devy = CGFloat(resulth * 1 / 4)
            for idx in 1..<4
            {
              var str: String = ""
              var reqred = false
              let battery_space = "      "
              let battery_space_measure = "FPS"

              //let device_on = true

                var kf:Float = 0

                if idx == 1
                 {
                   kf = Float(LelGetBatteryRemaining())

                   if ( saveImageNextRender )
                     { str = "" }
                   else if (!device_on || kf < 0 )
                     { str = "--% " + battery_space  }
                   else
                     {
                       if ( LelIsBatteryCharging () != 0 )
                         {
                            str =  "++" + "% " + battery_space
                            //str =  "++" + String (format:"%.0f", kf) + "% " + battery_space
                         }
                       else
                         {
                           str =  String (format:"%.0f", kf) + "% " + battery_space
                         }

                       if kf <= 20
                        { reqred = true }
                     }
                 }
                else if idx == 2
                 {
                   str = "" + String (LelGetDisplayFPS ()) + " FPS"

                    if buttonContrast?.backgroundColor == .black
                    { continue }
                 }
                else if idx == 3
                 {
                   str = "" + String (LelGetDataFrequency ()) + "Hz"
                 }

                if idx < 3     //idx = 1, 2: battery, FPS
                 {
                   let rw: CGFloat = 1000, rh: CGFloat = 1000
                   let txtsize = str.size(withAttributes: dev_small_textattrBlack)
                   let x:CGFloat = devx-txtsize.width
                   let y:CGFloat = devy


                   //x,y = upper-left corner; width & height is display boundary
                   let rect = CGRect(x:x, y:y, width:rw, height:rh)
                   let rect1 = CGRect(x:x-1, y:y-1, width:rw, height:rh)
                   let rect2 = CGRect(x:x-1, y:y+1, width:rw, height:rh)
                   let rect3 = CGRect(x:x+1, y:y-1, width:rw, height:rh)
                   let rect4 = CGRect(x:x+1, y:y+1, width:rw, height:rh)

                   str.draw(in: rect1, withAttributes: dev_small_textattrBlack)
                   str.draw(in: rect2, withAttributes: dev_small_textattrBlack)
                   str.draw(in: rect3, withAttributes: dev_small_textattrBlack)
                   str.draw(in: rect4, withAttributes: dev_small_textattrBlack)

                   if reqred
                   { str.draw(in: rect, withAttributes: dev_small_textattrRed) }
                   else
                   { str.draw(in: rect, withAttributes: dev_small_textattrGray) }

                     //graph of battery
                     if  idx == 1 && device_on && kf >= 0 && !saveImageNextRender
                      {
                         let txtsize = battery_space_measure.size(withAttributes: dev_small_textattrBlack)
                         let batsize_cx = txtsize.width
                         let batsize_cy = txtsize.height
                         let x:CGFloat = devx
                         let y:CGFloat = devy

                         let battery_headlen = batsize_cx * 1/6;
                         let battery_bodylen = batsize_cx * 5/6;
                         var brect_left, brect_top, brect_right, brect_bottom: CGFloat

                         let context = UIGraphicsGetCurrentContext()
                         var rectangle: CGRect

                         //battery body
                         brect_left = x - batsize_cx+battery_headlen;  brect_top = y + batsize_cy*0/5;
                         brect_right = brect_left + battery_bodylen; brect_bottom = y + batsize_cy * 5/5;
                         rectangle = CGRect(x: brect_left, y: brect_top, width: brect_right-brect_left, height: brect_bottom-brect_top)
                         context?.setFillColor(UIColor.black.cgColor)
                         context?.addRect(rectangle)
                         context?.drawPath(using: .fill)
                         brect_left+=1
                            brect_right-=1
                           brect_top+=1
                            brect_bottom-=1
                         rectangle = CGRect(x: brect_left, y: brect_top, width: brect_right-brect_left, height: brect_bottom-brect_top)
                         context?.setFillColor(UIColor.gray.cgColor)
                         context?.addRect(rectangle)
                         context?.drawPath(using: .fill)
                         //battery capacity
                         brect_left+=3
                         brect_right-=2
                         brect_top+=2
                         brect_bottom-=2
                         brect_right = brect_left +  ( (100 - CGFloat(kf)) * (brect_right-brect_left) / 100);
                         rectangle = CGRect(x: brect_left, y: brect_top, width: brect_right-brect_left, height: brect_bottom-brect_top)
                         context?.setFillColor(UIColor.black.cgColor)
                         context?.addRect(rectangle)
                         context?.drawPath(using: .fill)
                         //battery head
                         brect_left = x - batsize_cx;  brect_top = y + batsize_cy * 1/4;
                         brect_right = brect_left + battery_headlen + 1; brect_bottom = y + batsize_cy * 3/4;
                         rectangle = CGRect(x: brect_left, y: brect_top, width: brect_right-brect_left, height: brect_bottom-brect_top)
                         context?.setFillColor(UIColor.black.cgColor)
                         context?.addRect(rectangle)
                         context?.drawPath(using: .fill)
                         brect_left+=1
                         brect_top+=1
                         brect_bottom-=1
                         rectangle = CGRect(x: brect_left, y: brect_top, width: brect_right-brect_left, height: brect_bottom-brect_top)
                         context?.setFillColor(UIColor.gray.cgColor)
                         context?.addRect(rectangle)
                         context?.drawPath(using: .fill)

                      }  //idx == 1: draw battery

                   devy = devy+txtsize.height
                 }
                else  //idx == 3, Data Frequency
                 {
                   let rw: CGFloat = 1000, rh: CGFloat = 1000
                   let txtsize = str.size(withAttributes: dev_textattrBlack)
                   let x:CGFloat = devx-txtsize.width
                   let y:CGFloat = devy
                   devy = devy+txtsize.height

                   //x,y = upper-left corner; width & height is display boundary
                   let rect = CGRect(x:x, y:y, width:rw, height:rh)
                   let rect1 = CGRect(x:x-1, y:y-1, width:rw, height:rh)
                   let rect2 = CGRect(x:x-1, y:y+1, width:rw, height:rh)
                   let rect3 = CGRect(x:x+1, y:y-1, width:rw, height:rh)
                   let rect4 = CGRect(x:x+1, y:y+1, width:rw, height:rh)

                   str.draw(in: rect1, withAttributes: dev_textattrBlack)
                   str.draw(in: rect2, withAttributes: dev_textattrBlack)
                   str.draw(in: rect3, withAttributes: dev_textattrBlack)
                   str.draw(in: rect4, withAttributes: dev_textattrBlack)

                   str.draw(in: rect, withAttributes: dev_textattrGray)
                 }

            }



            //Annotations
            let idxmax: CInt = LelAnnotation_Max
            for idx in stride(from: 1, to: idxmax, by: 1)
            {
                let str = String(cString: LelGetAnnotationText (idx))
                let rw: CGFloat = 1000, rh: CGFloat = 1000
                let txtsize = str.size(withAttributes: textattrWhite)
                let x = CGFloat (LelGetAnnotationX (idx))-txtsize.width/2
                let y = CGFloat (LelGetAnnotationY (idx))-txtsize.height/2

                LelPutAnnotation(idx, CInt(x), CInt(y), CInt(txtsize.width), CInt(txtsize.height), 0)

                //x,y = upper-left corner; width & height is display boundary
                let rect = CGRect(x:x, y:y, width:rw, height:rh)
                let rect1 = CGRect(x:x-1, y:y-1, width:rw, height:rh)
                let rect2 = CGRect(x:x-1, y:y+1, width:rw, height:rh)
                let rect3 = CGRect(x:x+1, y:y-1, width:rw, height:rh)
                let rect4 = CGRect(x:x+1, y:y+1, width:rw, height:rh)

                str.draw(in: rect1, withAttributes: textattrBlack)
                str.draw(in: rect2, withAttributes: textattrBlack)
                str.draw(in: rect3, withAttributes: textattrBlack)
                str.draw(in: rect4, withAttributes: textattrBlack)

                if idx == LelGetLastAnnotationHolding()
                { str.draw(in: rect, withAttributes: textattrCyan) }
                else
                { str.draw(in: rect, withAttributes: textattrWhite) }
            }

            let resultimage = UIGraphicsGetImageFromCurrentImageContext()
            UIGraphicsEndImageContext()

            //Ouput to Screen
            //--------------------------------
            ultraSoundImageView?.image = resultimage

            var str: String = ""
            if zscale > 1.0
              { str = "[" + String(Int(zscale*100)) + "%] "}
            str = str + String(cString: LelGetStatusLine ())
            statusLine?.text = str

    }


    func showPoint (_ p: CGPoint)
    {
        /*
         let kdata = Int (getCData ())
         let korgstr = "abcd"
         let kstr = String (cString: getCStr(korgstr.cString(using: String.Encoding.utf8)))
         */

        let alertController = UIAlertController(title: "Leltek UltraSound", message: "Point\n \(p.x) \(p.y)", preferredStyle: UIAlertControllerStyle.alert)

        alertController.addAction(UIAlertAction(title: "Ok", style: UIAlertActionStyle.default, handler: nil))


        //alertController.addAction(UIAlertAction(title: "Cancel", style: UIAlertActionStyle.cancel, handler: nil))


        /*
         let btAdd = UIButton( frame: CGRect(x: 30, y: 50, width: 80, height: 30) )
         btAdd.backgroundColor = UIColor.gray
         btAdd.setTitle("Add", for: UIControlState())
         //btAdd.addTarget(self, action: #selector(ViewController.addview(_:)), for: UIControlEvents.touchUpInside)
         alertController.view.addSubview(btAdd)
         */



        //兩個輸入欄位
        /*
         alertController.addTextField {
         (textField: UITextField!) -> Void in
         textField.placeholder = "你的 Email"
         }

         alertController.addTextField {
         (textField: UITextField!) -> Void in
         textField.placeholder = "密碼"
         textField.isSecureTextEntry = true /*表示畫面不顯示正在輸入的密碼*/
         } */

        self.present(alertController, animated: true, completion: nil)

    }
    /**
     Cliff 0110 save image-----------------------------------------------------------
     */
    func saveImageDocumentDirectory(){

        let fileManager = FileManager.default
        let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        for index in 1...3{
            let path = documentDirectory.appending("/"+naviTitle!+"_"+String(index)+".png")
            if (!fileManager.fileExists(atPath: path)) {
                let imageData = UIImagePNGRepresentation(ultraSoundImageView!.image!)
                print(imageData!)
                print (path)
                let success: Bool = fileManager.createFile(atPath: path as String, contents: imageData, attributes: nil)
                if success {
                            print("file create")
                    if index == 1{
                        floatL1 = Float(LelGetFinalDis())
                        CalStruct.numL1 = floatL1
                        stringL1 = NSString(format:"%.1f",floatL1!) as String
                        CalStruct.showL1 = stringL1
                    }
                    else if index == 2{
                        floatL2 = Float(LelGetFinalDis())
                        CalStruct.numL2 = floatL1
                        stringL2 = NSString(format:"%.1f",floatL2!) as String
                        CalStruct.showL2 = stringL2
                    }
                    else if index == 3{
                        floatL3 = Float(LelGetFinalDis())
                        CalStruct.numL3 = floatL3
                        stringL3 = NSString(format:"%.1f",floatL3!) as String
                        CalStruct.showL3 = stringL3
                        
                    }
                    volCalculator()
                    calLine?.text = setCalLabel()
                    isimageload()
                            }
                else{
                    print("failed")
                    }
                
                print("break")
                break
                }
            
        }
    }
    func volCalculator(){
        GetCalStruct()
        rate = (setting! as NSString).floatValue
        floatVol = floatL1! * floatL2! * floatL3! * rate!
        stringVol = NSString (format:"%.1f", floatVol!) as String
        print (floatVol!)
        print ("Vol: "+stringVol!)
    }
    func setCalLabel()-> String{
        GetCalStruct()
        let text1 = stringL1!+"*"+stringL2!+"*"
        let text2 = stringL3!+"*"+setting!+"="+stringVol!
        return text1+text2
    }
    //save image----------------------------------------------------------------------
    //read
    func isimageload(){
        let fileManager = FileManager.default
        let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        let path1 = documentDirectory.appending("/"+naviTitle!+"_"+String(1)+".png")
        let path2 = documentDirectory.appending("/"+naviTitle!+"_"+String(2)+".png")
        let path3 = documentDirectory.appending("/"+naviTitle!+"_"+String(3)+".png")
        if (fileManager.fileExists(atPath: path1)) {
            if fullScreen
            { image1none?.isHidden = true
                image1?.isHidden = true }
            else if  buttonPadLayout == true && buttonPage == 0
            { image1none?.isHidden = true
                image1?.isHidden = false }
            else if  buttonPadLayout == false && buttonPage == 1
            { image1none?.isHidden = true
                image1?.isHidden = false }
            else
            { image1none?.isHidden = true
                image1?.isHidden = true}
        }
        else{
            if fullScreen
            { image1none?.isHidden = true
                image1?.isHidden = true }
            else if  buttonPadLayout == true && buttonPage == 0
            { image1none?.isHidden = false
                image1?.isHidden = true }
            else if  buttonPadLayout == false && buttonPage == 1
            { image1none?.isHidden = false
                image1?.isHidden = true }
            else
            { image1none?.isHidden = true
                image1?.isHidden = true}
        }
        if(fileManager.fileExists(atPath: path2)){
            if fullScreen
            { image2none?.isHidden = true
                image2?.isHidden = true }
            else if  buttonPadLayout == true && buttonPage == 0
            { image2none?.isHidden = true
                image2?.isHidden = false }
            else if  buttonPadLayout == false && buttonPage == 1
            { image2none?.isHidden = true
                image2?.isHidden = false }
            else
            { image2none?.isHidden = true
                image2?.isHidden = true}
           
        }
        else{
            if fullScreen
            { image2none?.isHidden = true
                image2?.isHidden = true }
            else if  buttonPadLayout == true && buttonPage == 0
            { image2none?.isHidden = false
                image2?.isHidden = true }
            else if  buttonPadLayout == false && buttonPage == 1
            { image2none?.isHidden = false
                image2?.isHidden = true }
            else
            { image2none?.isHidden = true
                image2?.isHidden = true}
        }
        if(fileManager.fileExists(atPath: path3)){
            if fullScreen
            { image3none?.isHidden = true
                image3?.isHidden = true }
            else if  buttonPadLayout == true && buttonPage == 0
            { image3none?.isHidden = true
                image3?.isHidden = false }
            else if  buttonPadLayout == false && buttonPage == 1
            { image3none?.isHidden = true
                image3?.isHidden = false }
            else
            { image3none?.isHidden = true
                image3?.isHidden = true}
        }
        else{
            if fullScreen
            { image3none?.isHidden = true
                image3?.isHidden = true }
            else if  buttonPadLayout == true && buttonPage == 0
            { image3none?.isHidden = false
                image3?.isHidden = true }
            else if  buttonPadLayout == false && buttonPage == 1
            { image3none?.isHidden = false
                image3?.isHidden = true }
            else
            { image3none?.isHidden = true
                image3?.isHidden = true}
        }
        if (fileManager.fileExists(atPath: path3)&&fileManager.fileExists(atPath: path2)&&fileManager.fileExists(atPath: path1))
        {
            buttonArchive?.isHidden = false
            print("showArchive")
        }
        else{
            buttonArchive?.isHidden = true
        }
    }
    func GetCalStruct (){
        if (CalStruct.showL1 != nil){
            stringL1 = CalStruct.showL1
        }
        if (CalStruct.showL2 != nil){
            stringL2 = CalStruct.showL2
        }
        if (CalStruct.showL3 != nil){
            stringL3 = CalStruct.showL3
        }
        if (CalStruct.showVol != nil){
            stringVol = CalStruct.showVol
        }
        if (CalStruct.numL1 != nil){
            floatL1 = CalStruct.numL1
        }
        if (CalStruct.numL2 != nil){
            floatL2 = CalStruct.numL2
        }
        if(CalStruct.numL3 != nil){
            floatL3 = CalStruct.numL3
        }
        if (CalStruct.numVol != nil){
            floatVol = CalStruct.numVol
        }
    }
    func archiveplist(){
        GetCalStruct()
        let fileManager = FileManager.default
        let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        let path = documentDirectory.appending("/"+naviTitle!+".plist")
        if (!fileManager.fileExists(atPath: path)) {
            let dicContent:[String: String] = ["Patient No": patientNo!, "Bed No":bedNo!,"Setting":setting!,"Organ": organ!,"L1": stringL1!, "L2": stringL2!, "L3":stringL3!, "Vol": stringVol!]
            let plistContent = NSDictionary(dictionary: dicContent)
            let success:Bool = plistContent.write(toFile: path, atomically: true)
            if success {
                print("file has been created!")
            }else{
                print("unable to create the file")
            }
        }
    }
}
