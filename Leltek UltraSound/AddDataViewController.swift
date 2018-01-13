//
//  AddDataViewController.swift
//  Leltek UltraSound
//
//  Created by ucfunnel on 2018/1/9.
//  Copyright © 2018年 Leltek. All rights reserved.
//

import UIKit

class AddDataViewController: UITableViewController, UITextFieldDelegate {
    
    var buttonBottomEdgeInVertical: CGFloat = 0
    
    //naviBar
    var naviBar : UINavigationBar?
    var buttonSave : UIBarButtonItem?

    
    // form
    var infoLabel, patientLabel, bedLabel, organLabel, settingLabel : UILabel?
    var patientField, bedField, settingField: UITextField?
    
    // organ drop list
    var organBtn: UIButton?
    var organView: UIView?
    
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
        prepareUI ((self.view?.bounds.size)!)
    }
    
    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
    }
    
    @objc func backtoMain() {
        self.present(ViewController(), animated: true, completion: nil)
    }
    

    func prepareUI (_ size: CGSize) {
        
        var upperspace: CGFloat = UIApplication.shared.statusBarFrame.height
        if upperspace < 44
        {  upperspace = 44}
        
        
        if naviBar ==  nil
        {
            let naviBar: UINavigationBar = UINavigationBar (frame: CGRect(x: 0, y: 0, width: size.width, height: 44))
            self.view.addSubview(naviBar);
            let naviItem = UINavigationItem(title:"Add Record");
            let saveItem = UIBarButtonItem(barButtonSystemItem: UIBarButtonSystemItem.save, target: self, action: #selector(saveAndBack));
            let backItem = UIBarButtonItem(barButtonSystemItem: UIBarButtonSystemItem.cancel, target:self, action: #selector(backtoMain));
            naviItem.rightBarButtonItem = saveItem;
            naviItem.leftBarButtonItem = backItem;
            naviBar.setItems([naviItem], animated: false);
        }
        if infoLabel == nil{
            let label: UILabel = UILabel(frame: CGRect(x:5, y:size.height*0.06, width: size.width, height: 45))
            label.textAlignment = .left
            label.textColor = UIColor.black
            label.text = "Information"
            label.font = UIFont(name:"HelveticaNeue-Bold", size: 24.0)
            self.view.addSubview(label)
            infoLabel = label
        }
        if patientLabel == nil{
            let label:UILabel = UILabel(frame: CGRect(x:5, y:size.height*0.12, width: size.width/2, height:45))
            label.textAlignment = .left
            label.textColor = UIColor.black
            label.text = "Patient No. :"
            label.font = UIFont(name:"HelveticaNeue", size: 20.0)
            self.view.addSubview(label)
            patientLabel = label
        }
        if patientField == nil{
            let field: UITextField = UITextField(frame:CGRect(x:size.width/2-5, y:size.height*0.12+2, width:size.width/2, height:40))
            field.borderStyle = .roundedRect
            field.placeholder = "7 digits combination"
            field.clearButtonMode = .whileEditing
            field.textColor = UIColor.black
            field.backgroundColor = UIColor.lightGray
            field.delegate = self
            self.view.addSubview(field)
            patientField = field
        }
        if bedLabel == nil{
            let label:UILabel = UILabel(frame: CGRect(x:5, y:size.height*0.18, width:size.width/2, height:45))
            label.textAlignment = .left
            label.textColor = UIColor.black
            label.text = "Bed No. :"
            label.font = UIFont(name:"HelveticaNeue", size: 20.0)
            self.view.addSubview(label)
            bedLabel = label
        }
        if bedField == nil{
            let field:UITextField = UITextField(frame: CGRect(x: size.width/2-5, y:size.height*0.18+2, width:size.width/2, height: 40))
            field.borderStyle = .roundedRect
            field.placeholder = "7 digits combination"
            field.clearButtonMode = .whileEditing
            field.backgroundColor = UIColor.lightGray
            field.textColor = UIColor.black
            field.delegate = self
            self.view.addSubview(field)
            bedField = field
        }
        if settingLabel == nil{
            let label:UILabel = UILabel(frame: CGRect(x:5, y: size.height*0.24, width: size.width/2, height:45))
            label.textAlignment = .left
            label.textColor = UIColor.black
            label.text = "Setting"
            label.font = UIFont(name:"HelveticaNeue", size: 20.0)
            self.view.addSubview(label)
            settingLabel = label
        }
        if settingField == nil{
            let field:UITextField = UITextField(frame: CGRect(x: size.width/2-5, y:size.height*0.24+2, width:size.width/2, height: 40))
            field.borderStyle = .roundedRect
            field.placeholder = "rate"
            field.clearButtonMode = .whileEditing
            field.backgroundColor = UIColor.lightGray
            field.textColor = UIColor.black
            field.delegate = self
            self.view.addSubview(field)
            settingField = field
        }
        if organLabel == nil{
            let label:UILabel = UILabel(frame:CGRect(x:5, y: size.height*0.3, width:size.width/2, height:45))
            label.textAlignment = .left
            label.textColor = UIColor.black
            label.text = "Organ"
            label.font = UIFont(name:"HelveticaNeue", size: 20.0)
            self.view.addSubview(label)
            organLabel = label
        }
        if organBtn == nil{
            let button = dropDownBtn.init(frame: CGRect(x:size.width/2-5, y:size.height*0.3, width:size.width/2, height:45))
            button.setTitle("Pick Organ", for: .normal)
            button.dropView.dropDownOptions = ["General", "Bladder"]
            self.view.addSubview(button)
            organBtn = button
        }
    }
    func textFieldShouldReturn(_ textField: UITextField) -> Bool {
        self.view.endEditing(true)
        
        return true
    }
    @objc func saveAndBack(){
        let patientNo = patientField?.text
        let bedNo = bedField?.text
        let setting = settingField?.text
        let organ = organBtn?.currentTitle
        //write plist
        let fileManager = FileManager.default
        let documentDirectory = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0] as String
        let path = documentDirectory.appending("/example.plist")
        
        if (!fileManager.fileExists(atPath: path)) {
            let dicContent:[String: String] = ["Patient No": patientNo!, "Bed No":bedNo!,"Setting":setting!,"Organ": organ!]
            let plistContent = NSDictionary(dictionary: dicContent)
            let success:Bool = plistContent.write(toFile: path, atomically: true)
            if success {
                print("file has been created!")
            }else{
                print("unable to create the file")
            }
            
        }else{
            let dicContent:[String: String] = ["Patient No": patientNo!, "Bed No":bedNo!,"Setting":setting!,"Organ": organ!]
            let plistContent = NSDictionary(dictionary: dicContent)
            let success:Bool = plistContent.write(toFile: path, atomically: true)
            if success {
                print("file has been created!")
            }else{
                print("unable to create the file")
            }
        }
        self.present(ViewController(), animated: true, completion: nil)
    }
}




protocol dropDownProtocol {
        func dropDownPress(string: String)
}
    
class dropDownBtn: UIButton, dropDownProtocol{
    
    var dropView = dropDownView()
    var height = NSLayoutConstraint()
    
    func dropDownPress(string: String) {
        self.setTitle(string, for: .normal)
        self.dismissDropDown()
    }
    
        
        override init(frame:CGRect){
            
            super.init(frame: frame)
            self.backgroundColor = UIColor.lightGray
            
            dropView = dropDownView.init(frame: CGRect(x:0, y:0, width:0, height:0))
            dropView.delegate = self
            dropView.translatesAutoresizingMaskIntoConstraints = false
            
        }
    override func didMoveToSuperview() {
        self.superview?.addSubview(dropView)
        self.superview?.bringSubview(toFront: dropView)
        
        dropView.topAnchor.constraint(equalTo: self.bottomAnchor).isActive = true
        dropView.centerXAnchor.constraint(equalTo: self.centerXAnchor).isActive = true
        dropView.widthAnchor.constraint(equalTo: self.widthAnchor).isActive = true
        height = dropView.heightAnchor.constraint(equalToConstant: 0)
    }
    
    
    var isOpen = false
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        if isOpen == false{
            isOpen = true
            NSLayoutConstraint.deactivate([self.height])
            
            if self.dropView.tableView.contentSize.height > 150{
                self.height.constant = 150
            }
            else{
                self.height.constant = self.dropView.tableView.contentSize.height
            }
           
            NSLayoutConstraint.activate([self.height])
            
            UIView.animate(withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.5, initialSpringVelocity: 0.5, options: .curveEaseInOut, animations: {
                self.dropView.layoutIfNeeded()
                self.dropView.center.y += self.dropView.frame.height/2
            }, completion: nil)
        }else{
            isOpen = false
            NSLayoutConstraint.deactivate([self.height])
            self.height.constant = 0
            NSLayoutConstraint.activate([self.height])
            
            UIView.animate(withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.5, initialSpringVelocity: 0.5, options: .curveEaseInOut, animations: {
                self.dropView.center.y -= self.dropView.frame.height/2
                self.dropView.layoutIfNeeded()
        }, completion: nil)
    
        }
    }
    func dismissDropDown(){
        isOpen = false
        NSLayoutConstraint.deactivate([self.height])
        self.height.constant = 0
        NSLayoutConstraint.activate([self.height])
        
        UIView.animate(withDuration: 0.5, delay: 0, usingSpringWithDamping: 0.5, initialSpringVelocity: 0.5, options: .curveEaseInOut, animations: {
            self.dropView.center.y -= self.dropView.frame.height/2
            self.dropView.layoutIfNeeded()
        }, completion: nil)
        
    }
        required init?(coder aDecoder: NSCoder) {
            fatalError("init(coder:) has not been implemented")
        }
    
    
    
    
    }
    class dropDownView: UIView, UITableViewDelegate, UITableViewDataSource{
        
        var dropDownOptions = [String]()
        var tableView = UITableView()
        
        var delegate : dropDownProtocol!
        
        override init(frame: CGRect) {
            super.init(frame: frame)
            tableView.backgroundColor = UIColor.lightGray
            self.backgroundColor = UIColor.lightGray
            
            
            tableView.delegate = self
            tableView.dataSource = self
            
            tableView.translatesAutoresizingMaskIntoConstraints = false
            
            self.addSubview(tableView)
            tableView.leftAnchor.constraint(equalTo: leftAnchor).isActive = true
            tableView.rightAnchor.constraint(equalTo: rightAnchor).isActive = true
            tableView.topAnchor.constraint(equalTo: topAnchor).isActive = true
            tableView.bottomAnchor.constraint(equalTo: bottomAnchor).isActive = true
            
        }
        
        required init?(coder aDecoder: NSCoder) {
            fatalError("init(coder:) has not been implemented")
        }
        
        func numberOfSections(in tableView: UITableView) -> Int {
            return 1
        }
        
        func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
            return dropDownOptions.count
        }
        func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
            let cell = UITableViewCell()
            cell.textLabel?.text = dropDownOptions[indexPath.row]
            cell.backgroundColor = UIColor.lightGray
            return cell
        }
        func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
           self.delegate.dropDownPress(string: dropDownOptions[indexPath.row])
            self.tableView.deselectRow(at: indexPath, animated: true)
        }
        
    }

