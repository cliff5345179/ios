//
//  image1ViewController.swift
//  Leltek UltraSound
//
//  Created by ucfunnel on 2018/1/11.
//  Copyright © 2018年 Leltek. All rights reserved.
//

import UIKit

class Image1ViewController: UIViewController{
    //naviBar
    var naviBar : UINavigationBar?
    var buttonDelete : UIBarButtonItem?
    
    var imageView : UIImageView?
    var naviTitle = "ultraSound" as String?
    var organ: String?
    
    var buttonBottomEdgeInVertical: CGFloat = 0
    
    override func viewDidLoad() {
        
        super.viewDidLoad()
        
        view.backgroundColor = .white
        
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
        readFromPlist()

        prepareUI ((self.view?.bounds.size)!)
       
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }
    
    func prepareUI (_ size: CGSize) {
        
        var upperspace: CGFloat = UIApplication.shared.statusBarFrame.height
        if upperspace < 44
        {  upperspace = 44}
        
        
        if naviBar ==  nil
        {
            let naviBar: UINavigationBar = UINavigationBar (frame: CGRect(x: 0, y: 0, width: size.width, height: 44))
            self.view.addSubview(naviBar);
            let naviItem = UINavigationItem(title:"image 1");
            let deleteItem = UIBarButtonItem(barButtonSystemItem: UIBarButtonSystemItem.trash, target: self, action: #selector(deleteImage));
            let backItem = UIBarButtonItem(barButtonSystemItem: UIBarButtonSystemItem.cancel, target:self, action: #selector(backtoMain));
            naviItem.rightBarButtonItem = deleteItem;
            naviItem.leftBarButtonItem = backItem;
            naviBar.setItems([naviItem], animated: false);
        }
        if imageView == nil
        {
            let imageView = UIImageView()
            imageView.frame = CGRect(x: 0, y: upperspace, width: size.width, height: size.height-upperspace)
            let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
            let path = documentDirectory.appending("/"+naviTitle!+"_"+String(1)+".png")
            let image = UIImage(contentsOfFile: path)
            imageView.image = image
            imageView.isUserInteractionEnabled = true //Enable touch
            imageView.isMultipleTouchEnabled = true
            
            //imageView.layer.borderWidth = CGFloat(5);
            //imageView.layer.borderColor = UIColor.lightGray.cgColor
            self.view.addSubview(imageView)
        }
    }
    
    @objc func backtoMain() {
        self.present(ViewController(), animated: true, completion: nil)
    }
    @objc func deleteImage(_ sender: Any?) {
        CalStruct.showL1 = "0"
        CalStruct.numL1 = 0
        let fileManager = FileManager.default
        let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        let path = documentDirectory.appending("/"+naviTitle!+"_"+String(1)+".png")
        do{
            if fileManager.fileExists(atPath: path){
                try fileManager.removeItem(atPath: path)
                self.present(ViewController(), animated: true, completion: nil)
            }
        }catch {
            print("failed")
        }
    }
   
    func readFromPlist(){
        var patientNo: String?
        
        let fileManager = FileManager.default
        let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        let path = documentDirectory.appending("/example.plist")
        if fileManager.fileExists(atPath: path){
            let dataDict = NSDictionary(contentsOfFile: path)
            if let dict = dataDict{
                patientNo = dict.object(forKey: "Patient No") as? String
                organ = dict.object(forKey: "Organ")as? String
                naviTitle = "Patient No: "+patientNo!+" Organ: "+organ!
                print("patient No:",patientNo!)
            }
            else{
                naviTitle = "UltraSound"
                print("load failed")
            }
        }
    }
}
